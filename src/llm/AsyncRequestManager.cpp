#include "AsyncRequestManager.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace OpenMind {

AsyncRequestManager::AsyncRequestManager() {}

AsyncRequestManager::~AsyncRequestManager() {
    shutdown();
}

void AsyncRequestManager::initialize(int maxConc, int rateLimit) {
    maxConcurrent = maxConc;
    rateLimitPerMinute = rateLimit;
    running = true;
    for (int i = 0; i < maxConcurrent; i++) {
        workers.emplace_back(&AsyncRequestManager::workerThread, this);
    }
}

void AsyncRequestManager::shutdown() {
    running = false;
    queueCondition.notify_all();
    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }
    workers.clear();
}

void AsyncRequestManager::registerClient(LLMProvider provider, std::shared_ptr<LLMInterface> client) {
    clients[provider] = client;
}

std::string AsyncRequestManager::submitRequest(const std::string& prompt,
                                               const std::string& systemInstruction,
                                               LLMProvider provider,
                                               RequestPriority priority,
                                               AsyncCallback callback) {
    std::stringstream ss;
    ss << "req_" << std::setfill('0') << std::setw(6) << nextId.fetch_add(1);
    std::string id = ss.str();

    AsyncRequest req;
    req.id = id;
    req.prompt = prompt;
    req.systemInstruction = systemInstruction;
    req.provider = provider;
    req.priority = priority;
    req.maxRetries = 3;
    req.createdAt = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(responseMutex);
        responses[id] = AsyncResponse{id, LLMResponse{}, false};
        responses[id].result.error = "Pending";
    }

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        auto it = requestQueue.begin();
        while (it != requestQueue.end() && it->priority >= priority) ++it;
        requestQueue.insert(it, req);
    }

    queueCondition.notify_one();
    return id;
}

bool AsyncRequestManager::cancelRequest(const std::string& requestId) {
    std::lock_guard<std::mutex> lock(queueMutex);
    for (auto it = requestQueue.begin(); it != requestQueue.end(); ++it) {
        if (it->id == requestId) {
            requestQueue.erase(it);
            std::lock_guard<std::mutex> rlock(responseMutex);
            responses[requestId].completed = true;
            responses[requestId].result.error = "Cancelled";
            return true;
        }
    }
    return false;
}

bool AsyncRequestManager::isRequestComplete(const std::string& requestId) const {
    std::lock_guard<std::mutex> lock(responseMutex);
    auto it = responses.find(requestId);
    return it != responses.end() && it->second.completed;
}

AsyncResponse AsyncRequestManager::getResponse(const std::string& requestId) const {
    std::lock_guard<std::mutex> lock(responseMutex);
    auto it = responses.find(requestId);
    if (it != responses.end()) return it->second;
    return AsyncResponse{requestId, LLMResponse{}, false};
}

std::vector<AsyncResponse> AsyncRequestManager::getCompletedRequests() const {
    std::lock_guard<std::mutex> lock(responseMutex);
    std::vector<AsyncResponse> result;
    for (auto& [id, resp] : responses) {
        if (resp.completed && resp.result.success) result.push_back(resp);
    }
    return result;
}

std::vector<AsyncResponse> AsyncRequestManager::getFailedRequests() const {
    std::lock_guard<std::mutex> lock(responseMutex);
    std::vector<AsyncResponse> result;
    for (auto& [id, resp] : responses) {
        if (resp.completed && !resp.result.success) result.push_back(resp);
    }
    return result;
}

bool AsyncRequestManager::canMakeRequest() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    auto now = std::chrono::steady_clock::now();
    auto oneMinuteAgo = now - std::chrono::minutes(1);
    int recentRequests = 0;
    for (auto& t : requestTimes) {
        if (t > oneMinuteAgo) recentRequests++;
    }
    return recentRequests < rateLimitPerMinute.load();
}

void AsyncRequestManager::recordRequestTime() {
    std::lock_guard<std::mutex> lock(queueMutex);
    requestTimes.push_back(std::chrono::steady_clock::now());
    auto oneMinuteAgo = std::chrono::steady_clock::now() - std::chrono::minutes(1);
    requestTimes.erase(
        std::remove_if(requestTimes.begin(), requestTimes.end(),
                       [&oneMinuteAgo](auto& t) { return t < oneMinuteAgo; }),
        requestTimes.end());
}

void AsyncRequestManager::processQueue() {
    std::lock_guard<std::mutex> lock(queueMutex);
    queueCondition.notify_all();
}

void AsyncRequestManager::workerThread() {
    while (running) {
        AsyncRequest request;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            queueCondition.wait(lock, [this] {
                return !running || (!requestQueue.empty() && activeCount < maxConcurrent);
            });
            if (!running) return;
            if (requestQueue.empty()) continue;

            request = requestQueue.front();
            requestQueue.erase(requestQueue.begin());
            activeCount++;
        }

        bool retried = processRequest(request);

        if (!retried) {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                activeCount--;
            }
            queueCondition.notify_one();
        }
    }
}

bool AsyncRequestManager::processRequest(AsyncRequest& request) {
    if (!canMakeRequest()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    auto clientIt = clients.find(request.provider);
    if (clientIt == clients.end()) {
        std::lock_guard<std::mutex> lock(responseMutex);
        responses[request.id].completed = true;
        responses[request.id].result.success = false;
        responses[request.id].result.error = "No client registered for provider";
        failedCount++;
        return false;
    }

    recordRequestTime();
    auto start = std::chrono::steady_clock::now();

    LLMResponse resp = clientIt->second->sendPrompt(request.prompt, request.systemInstruction);

    auto elapsed = std::chrono::steady_clock::now() - start;
    float latencyMs = (float)std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();

    if (!resp.success && request.retryCount < request.maxRetries) {
        request.retryCount++;
        std::lock_guard<std::mutex> lock(queueMutex);
        requestQueue.push_back(request);
        queueCondition.notify_one();
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(responseMutex);
        responses[request.id].completed = true;
        responses[request.id].result = resp;
    }

    if (resp.success) completedCount++;
    else failedCount++;

    responseCondition.notify_all();
    return false;
}

int AsyncRequestManager::getPendingCount() const {
    std::lock_guard<std::mutex> lock(queueMutex);
    return (int)requestQueue.size();
}
int AsyncRequestManager::getActiveCount() const { return activeCount.load(); }
int AsyncRequestManager::getCompletedCount() const { return completedCount.load(); }
int AsyncRequestManager::getFailedCount() const { return failedCount.load(); }

} // namespace OpenMind
