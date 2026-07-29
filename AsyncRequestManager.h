#pragma once

#include "LLMInterface.h"
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <chrono>

namespace OpenMind {

enum class RequestPriority {
    LOW = 0,
    NORMAL = 1,
    HIGH = 2,
    CRITICAL = 3
};

struct AsyncRequest {
    std::string id;
    std::string prompt;
    std::string systemInstruction;
    LLMProvider provider;
    RequestPriority priority = RequestPriority::NORMAL;
    int retryCount = 0;
    int maxRetries = 3;
    std::chrono::steady_clock::time_point createdAt;
};

struct AsyncResponse {
    std::string requestId;
    LLMResponse result;
    bool completed = false;
};

using AsyncCallback = std::function<void(const AsyncResponse&)>;

class AsyncRequestManager {
public:
    AsyncRequestManager();
    ~AsyncRequestManager();

    void initialize(int maxConcurrent = 4, int rateLimitPerMinute = 60);
    void shutdown();

    std::string submitRequest(const std::string& prompt,
                              const std::string& systemInstruction,
                              LLMProvider provider,
                              RequestPriority priority = RequestPriority::NORMAL,
                              AsyncCallback callback = nullptr);

    bool cancelRequest(const std::string& requestId);
    bool isRequestComplete(const std::string& requestId) const;
    AsyncResponse getResponse(const std::string& requestId) const;
    std::vector<AsyncResponse> getCompletedRequests() const;
    std::vector<AsyncResponse> getFailedRequests() const;

    void registerClient(LLMProvider provider, std::shared_ptr<LLMInterface> client);
    void processQueue();
    void setMaxConcurrent(int max);
    void setRateLimit(int perMinute);
    int getPendingCount() const;
    int getActiveCount() const;
    int getCompletedCount() const;
    int getFailedCount() const;

private:
    void workerThread();
    void processRequest(AsyncRequest& request);
    bool canMakeRequest() const;
    void recordRequestTime();

    std::map<LLMProvider, std::shared_ptr<LLMInterface>> clients;
    std::vector<AsyncRequest> requestQueue;
    std::map<std::string, AsyncResponse> responses;

    mutable std::mutex queueMutex;
    mutable std::mutex responseMutex;
    std::condition_variable queueCondition;
    std::condition_variable responseCondition;

    std::vector<std::thread> workers;
    std::atomic<bool> running{false};
    std::atomic<int> activeCount{0};
    int maxConcurrent = 4;
    int rateLimitPerMinute = 60;
    std::vector<std::chrono::steady_clock::time_point> requestTimes;

    std::atomic<int> completedCount{0};
    std::atomic<int> failedCount{0};
    std::atomic<int> nextId{0};
};

} // namespace OpenMind
