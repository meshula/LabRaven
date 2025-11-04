#ifndef Providers_LlamaCpp_CurlClient_hpp
#define Providers_LlamaCpp_CurlClient_hpp

#include <string>
#include <curl/curl.h>

namespace lab {

/**
 * RAII wrapper for libcurl HTTP client.
 * Handles initialization, cleanup, and safe HTTP POST requests.
 * Thread-safe per-instance (each instance has its own CURL handle).
 */
class CurlClient {
public:
    CurlClient();
    ~CurlClient();
    
    // Delete copy/move to prevent handle sharing
    CurlClient(const CurlClient&) = delete;
    CurlClient& operator=(const CurlClient&) = delete;
    CurlClient(CurlClient&&) = delete;
    CurlClient& operator=(CurlClient&&) = delete;
    
    /**
     * Perform synchronous HTTP POST with JSON payload.
     * @param url Target URL (e.g., "http://localhost:8080/v1/chat/completions")
     * @param jsonPayload JSON string to send as request body
     * @param timeoutSeconds Request timeout (default: 30 seconds)
     * @return Response body as string
     * @throws std::runtime_error on curl errors or HTTP failures
     */
    std::string post(const std::string& url, 
                     const std::string& jsonPayload,
                     long timeoutSeconds = 30);
    
    /**
     * Check if curl handle is valid.
     */
    bool isValid() const { return curl_ != nullptr; }
    
private:
    CURL* curl_;
    
    // Static callback for curl to write response data
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

} // lab

#endif // Providers_LlamaCpp_CurlClient_hpp