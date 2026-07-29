#include "HttpClient.h"
#include <sstream>
#include <algorithm>
#include <vector>

namespace OpenMind {

HttpClient::HttpClient() {}
HttpClient::~HttpClient() {}

void HttpClient::setTimeout(int milliseconds) { timeoutMs = milliseconds; }
void HttpClient::setMaxRetries(int retries) { maxRetries = retries; }

#ifdef _WIN32

std::wstring HttpClient::toWide(const std::string& str) {
    if (str.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    std::wstring wstr(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], len);
    return wstr;
}

bool HttpClient::parseUrl(const std::wstring& url, std::wstring& host,
                          std::wstring& path, int& port, bool& useHttps) {
    useHttps = (url.substr(0, 8) == L"https://");
    std::wstring rest = useHttps ? url.substr(8) : url.substr(7);

    size_t pathStart = rest.find(L'/');
    std::wstring hostPort = (pathStart != std::wstring::npos) ? rest.substr(0, pathStart) : rest;
    path = (pathStart != std::wstring::npos) ? rest.substr(pathStart) : L"/";

    size_t colonPos = hostPort.find(L':');
    if (colonPos != std::wstring::npos) {
        host = hostPort.substr(0, colonPos);
        port = std::stoi(hostPort.substr(colonPos + 1));
    } else {
        host = hostPort;
        port = useHttps ? 443 : 80;
    }
    return true;
}

HttpResponse HttpClient::requestWinHTTP(const std::wstring& method, const std::wstring& url,
                                        const std::wstring& extraHeaders, const std::string& body) {
    HttpResponse resp;
    std::wstring host, path;
    int port;
    bool useHttps;

    if (!parseUrl(url, host, path, port, useHttps)) {
        resp.error = "Failed to parse URL";
        return resp;
    }

    HINTERNET hSession = WinHttpOpen(L"OpenMind/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { resp.error = "WinHttpOpen failed"; return resp; }

    WinHttpSetTimeouts(hSession, 0, timeoutMs, timeoutMs, timeoutMs);

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), (INTERNET_PORT)port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); resp.error = "WinHttpConnect failed"; return resp; }

    DWORD flags = useHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(),
                                            nullptr, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
                     resp.error = "WinHttpOpenRequest failed"; return resp; }

    if (useHttps) {
        DWORD securityFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                              SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                              SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                              SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &securityFlags, sizeof(securityFlags));
    }

    BOOL sent = WinHttpSendRequest(hRequest,
                                   extraHeaders.c_str(),
                                   (DWORD)extraHeaders.size(),
                                   body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.c_str(),
                                   (DWORD)body.size(),
                                   (DWORD)body.size(), 0);

    if (!sent) { WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect);
                 WinHttpCloseHandle(hSession); resp.error = "WinHttpSendRequest failed"; return resp; }

    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession); resp.error = "WinHttpReceiveResponse failed"; return resp;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX);
    resp.statusCode = (int)statusCode;

    DWORD headerSize = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                           WINHTTP_HEADER_NAME_BY_INDEX, nullptr, 0, WINHTTP_NO_HEADER_INDEX);
    if (headerSize > 0) {
        std::wstring rawHeaders(headerSize / sizeof(wchar_t), 0);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
                            WINHTTP_HEADER_NAME_BY_INDEX, &rawHeaders[0], &headerSize, WINHTTP_NO_HEADER_INDEX);
        std::wistringstream hdrStream(rawHeaders);
        std::wstring line;
        while (std::getline(hdrStream, line)) {
            size_t colon = line.find(L':');
            if (colon != std::wstring::npos) {
                std::wstring key = line.substr(0, colon);
                std::wstring val = line.substr(colon + 2);
                if (!val.empty() && val.back() == L'\r') val.pop_back();
                std::string sKey(key.begin(), key.end());
                std::string sVal(val.begin(), val.end());
                resp.headers[sKey] = sVal;
            }
        }
    }

    DWORD bytesAvailable = 0;
    while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
        std::vector<char> buffer(bytesAvailable);
        DWORD bytesRead = 0;
        WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead);
        resp.body.append(buffer.data(), bytesRead);
    }

    resp.success = (resp.statusCode >= 200 && resp.statusCode < 300);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return resp;
}

HttpResponse HttpClient::get(const std::string& url, const HttpHeaderMap& headers) {
    std::wstring wHeaders;
    for (auto& [k, v] : headers) {
        wHeaders += toWide(k) + L": " + toWide(v) + L"\r\n";
    }
    return requestWinHTTP(L"GET", toWide(url), wHeaders, "");
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body,
                              const std::string& contentType, const HttpHeaderMap& headers) {
    std::wstring wHeaders = L"Content-Type: " + toWide(contentType) + L"\r\n";
    for (auto& [k, v] : headers) {
        wHeaders += toWide(k) + L": " + toWide(v) + L"\r\n";
    }
    return requestWinHTTP(L"POST", toWide(url), wHeaders, body);
}

HttpResponse HttpClient::put(const std::string& url, const std::string& body,
                             const std::string& contentType, const HttpHeaderMap& headers) {
    std::wstring wHeaders = L"Content-Type: " + toWide(contentType) + L"\r\n";
    for (auto& [k, v] : headers) {
        wHeaders += toWide(k) + L": " + toWide(v) + L"\r\n";
    }
    return requestWinHTTP(L"PUT", toWide(url), wHeaders, body);
}

HttpResponse HttpClient::del(const std::string& url, const HttpHeaderMap& headers) {
    std::wstring wHeaders;
    for (auto& [k, v] : headers) {
        wHeaders += toWide(k) + L": " + toWide(v) + L"\r\n";
    }
    return requestWinHTTP(L"DELETE", toWide(url), wHeaders, "");
}

#else

HttpResponse HttpClient::get(const std::string& url, const HttpHeaderMap& headers) {
    HttpResponse resp;
    resp.error = "HTTP not implemented for this platform";
    return resp;
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body,
                              const std::string& contentType, const HttpHeaderMap& headers) {
    HttpResponse resp;
    resp.error = "HTTP not implemented for this platform";
    return resp;
}

HttpResponse HttpClient::put(const std::string& url, const std::string& body,
                             const std::string& contentType, const HttpHeaderMap& headers) {
    HttpResponse resp;
    resp.error = "HTTP not implemented for this platform";
    return resp;
}

HttpResponse HttpClient::del(const std::string& url, const HttpHeaderMap& headers) {
    HttpResponse resp;
    resp.error = "HTTP not implemented for this platform";
    return resp;
}

#endif

} // namespace OpenMind
