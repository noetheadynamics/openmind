#pragma once

#include <string>
#include <map>
#include <functional>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <winhttp.h>
    #pragma comment(lib, "winhttp.lib")
#endif

namespace OpenMind {

struct HttpResponse {
    int statusCode = 0;
    std::string body;
    std::map<std::string, std::string> headers;
    bool success = false;
    std::string error;
};

using HttpHeaderMap = std::map<std::string, std::string>;

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    HttpResponse get(const std::string& url, const HttpHeaderMap& headers = {});
    HttpResponse post(const std::string& url, const std::string& body,
                      const std::string& contentType = "application/json",
                      const HttpHeaderMap& headers = {});
    HttpResponse put(const std::string& url, const std::string& body,
                     const std::string& contentType = "application/json",
                     const HttpHeaderMap& headers = {});
    HttpResponse del(const std::string& url, const HttpHeaderMap& headers = {});

    void setTimeout(int milliseconds);
    void setMaxRetries(int retries);

private:
#ifdef _WIN32
    HttpResponse requestWinHTTP(const std::wstring& method, const std::wstring& url,
                                const std::wstring& headers, const std::string& body);
    bool parseUrl(const std::wstring& url, std::wstring& host, std::wstring& path,
                  int& port, bool& useHttps);
    std::wstring toWide(const std::string& str);
#endif

    int timeoutMs = 30000;
    int maxRetries = 3;
};

} // namespace OpenMind
