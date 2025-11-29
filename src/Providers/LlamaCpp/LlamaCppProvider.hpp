#ifndef Providers_LlamaCpp_hpp
#define Providers_LlamaCpp_hpp

#include "Lab/StudioCore.hpp"
#include "CurlClient.hpp"
#include <memory>
#include <string>
#include <vector>

namespace lab {

/**
 * Chat message structure for conversation history.
 */
struct ChatMessage {
    std::string role;      // "system", "user", or "assistant"
    std::string content;
};

/**
 * LlamaCpp Provider - Singleton service for llama.cpp server integration.
 * 
 * Follows LabRaven's Studio-Activity-Provider pattern.
 * Manages conversation state, HTTP communication, and CSP integration.
 * 
 * Key improvements from feedback:
 * - Full conversation history (not just last message)
 * - Proper temperature/top_p/top_k parameters
 * - Context window management
 * - Non-blocking via CSP state machine
 * - Robust HTTP via libcurl
 * - Proper JSON via RapidJSON
 */
class LlamaCppProvider : public Provider {
public:
    /**
     * Configuration options for llama.cpp server.
     * Defaults follow external feedback recommendations.
     */
    struct Options {
        std::string host = "127.0.0.1";
        uint16_t port = 8009;
        
        // Sampling parameters (optimized for instruction-tuned models)
        float temperature = 0.3f;      // Lower = more deterministic
        float top_p = 0.9f;             // Nucleus sampling
        int top_k = 40;                 // Candidate limiting
        int max_tokens = 256;           // Tokens per response
        std::vector<std::string> stop = {"</s>"};  // Stop sequences
        
        // Context management
        int max_context_tokens = 4096;  // Model's context window
    };
    
    LlamaCppProvider();
    ~LlamaCppProvider();
    
    // Provider interface
    static LlamaCppProvider* instance();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "LlamaCpp"; }
    
    // Configuration
    void setOptions(const Options& opts);
    const Options& getOptions() const;
    
    // Conversation management
    void addSystemMessage(const std::string& content);
    void clearHistory();
    const std::vector<ChatMessage>& getHistory() const;
    
    /**
     * Request chat completion (non-blocking).
     * Call from Activity, response comes back via CSP.
     * @param userMessage User's input message
     */
    void requestChatCompletion(const std::string& userMessage);
    
    /**
     * Poll for response (call from Activity's update loop).
     * @return Response string if available, empty string if still waiting
     */
    std::string pollResponse();
    
    /**
     * Check if a request is in progress.
     */
    bool isProcessing() const;
    
    // JSON payload methods (public for CSP module access)
    std::string buildChatCompletionPayload(const std::vector<ChatMessage>& history) const;
    std::string parseAssistantResponse(const std::string& jsonResponse) const;
    
    
    // History management (public for CSP module access)
    void addMessageToHistory(const ChatMessage& msg);
    void trimHistoryToFit();
    int estimateTokenCount(const std::vector<ChatMessage>& history) const;
    
    /**
     * Push response from CSP module (called by worker thread).
     * Thread-safe method for CSP module to enqueue responses.
     */
    void pushResponse(const std::string& response);
    void pushError(const std::string& error);
    
private:
    
private:
    struct data;
    data* _self;
    
    static LlamaCppProvider* _instance;
};

} // lab

#endif // Providers_LlamaCpp_hpp