#include "CurlClient.hpp"
#include <stdexcept>
#include <sstream>

namespace lab {

// Static callback for curl to write response data
size_t CurlClient::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

CurlClient::CurlClient() : curl_(nullptr) {
    curl_ = curl_easy_init();
    if (!curl_) {
        throw std::runtime_error("CurlClient: curl_easy_init() failed");
    }
}

CurlClient::~CurlClient() {
    if (curl_) {
        curl_easy_cleanup(curl_);
        curl_ = nullptr;
    }
}

std::string CurlClient::post(const std::string& url, 
                              const std::string& jsonPayload,
                              long timeoutSeconds) {
    if (!curl_) {
        throw std::runtime_error("CurlClient: invalid curl handle");
    }
    
    std::string response;
    
    // Reset curl handle for reuse
    curl_easy_reset(curl_);
    
    // Set URL
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    
    // Set POST method
    curl_easy_setopt(curl_, CURLOPT_POST, 1L);
    
    // Set POST data
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, jsonPayload.c_str());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, static_cast<long>(jsonPayload.size()));
    
    // Set headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    
    // Set write callback
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
    
    // Set timeout
    curl_easy_setopt(curl_, CURLOPT_TIMEOUT, timeoutSeconds);
    
    // Enable TCP keepalive
    curl_easy_setopt(curl_, CURLOPT_TCP_KEEPALIVE, 1L);
    
    // Follow redirects
    curl_easy_setopt(curl_, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_, CURLOPT_MAXREDIRS, 5L);
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl_);
    
    // Clean up headers
    curl_slist_free_all(headers);
    
    // Check for errors
    if (res != CURLE_OK) {
        std::ostringstream oss;
        oss << "CurlClient: HTTP request failed: " << curl_easy_strerror(res);
        throw std::runtime_error(oss.str());
    }
    
    // Check HTTP response code
    long httpCode = 0;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &httpCode);
    if (httpCode < 200 || httpCode >= 300) {
        std::ostringstream oss;
        oss << "CurlClient: HTTP error " << httpCode;
        throw std::runtime_error(oss.str());
    }
    
    return response;
}

} // lab