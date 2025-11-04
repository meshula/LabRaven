#include "LlamaCppProvider.hpp"
#include "LlamaCppCSPModule.hpp"
#include "Lab/LabApp.hpp"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <algorithm>
#include <numeric>

namespace lab {

// Pimpl data structure
struct LlamaCppProvider::data {
    Options options;
    std::vector<ChatMessage> history;
    std::mutex historyMutex;  // Protect history from CSP worker thread
    
    // Response queue for Activity polling
    std::queue<std::string> responseQueue;
    std::mutex queueMutex;
    
    // CSP state machine (direct member, not pointer)
    LlamaCppCSPModule cspModule;
    
    data(LlamaCppProvider* provider) 
        : cspModule(*LabApp::instance()->csp(), provider)
    {
        cspModule.Register();
    }
};

// Static singleton instance
LlamaCppProvider* LlamaCppProvider::_instance = nullptr;
LlamaCppProvider::LlamaCppProvider()
    : Provider(sname())
    , _self(new data(this))
{
    _instance = this;
}
{
    _instance = this;
}

LlamaCppProvider::~LlamaCppProvider() {
    delete _self;
    if (_instance == this) {
        _instance = nullptr;
    }
}

LlamaCppProvider* LlamaCppProvider::instance() {
    return _instance;
}

void LlamaCppProvider::setOptions(const Options& opts) {
    _self->options = opts;
}

const LlamaCppProvider::Options& LlamaCppProvider::getOptions() const {
    return _self->options;
}

void LlamaCppProvider::addSystemMessage(const std::string& content) {
    ChatMessage msg;
    msg.role = "system";
    msg.content = content;
    _self->history.push_back(msg);
}

void LlamaCppProvider::clearHistory() {
    _self->history.clear();
}

const std::vector<ChatMessage>& LlamaCppProvider::getHistory() const {
    return _self->history;
}

bool LlamaCppProvider::isProcessing() const {
    return _self->cspModule.isProcessing();
}

void LlamaCppProvider::requestChatCompletion(const std::string& userMessage) {
    // Simple delegation to CSP state machine
    // CSP module will handle threading, HTTP, and state management
    _self->cspModule.triggerRequest(userMessage);
}
        std::lock_guard<std::mutex> lock(_self->stateMutex);
        _self->processing = false;
    }
}

std::string LlamaCppProvider::pollResponse() {
    std::lock_guard<std::mutex> lock(_self->queueMutex);
    if (_self->responseQueue.empty()) {
        return "";
    }
    std::string response = _self->responseQueue.front();
    _self->responseQueue.pop();
    return response;

// Methods for CSP module to push responses back
void LlamaCppProvider::pushResponse(const std::string& response) {
    // Add to history
    ChatMessage msg;
    msg.role = "assistant";
    msg.content = response;
    {
        std::lock_guard<std::mutex> lock(_self->historyMutex);
        _self->history.push_back(msg);
    }
    
    // Push to response queue for Activity polling
    {
        std::lock_guard<std::mutex> lock(_self->queueMutex);
        _self->responseQueue.push(response);
    }
}

void LlamaCppProvider::pushError(const std::string& error) {
    std::lock_guard<std::mutex> lock(_self->queueMutex);
    _self->responseQueue.push(error);
}

void LlamaCppProvider::addMessageToHistory(const ChatMessage& msg) {
    std::lock_guard<std::mutex> lock(_self->historyMutex);
    _self->history.push_back(msg);
}
}

// Context window management (from feedback)
int LlamaCppProvider::estimateTokenCount(const std::vector<ChatMessage>& history) const {
    // Simple heuristic: 1 token ≈ 4 characters for English text
    return std::accumulate(history.begin(), history.end(), 0,
        [](int sum, const ChatMessage& msg) {
            return sum + static_cast<int>(msg.content.size() / 4);
        });
}

void LlamaCppProvider::trimHistoryToFit() {
    int approxTokens = estimateTokenCount(_self->history);
    
    // Keep removing oldest non-system messages until we fit
    while (approxTokens > _self->options.max_context_tokens && _self->history.size() > 1) {
        // Find first non-system message
        auto it = std::find_if(_self->history.begin(), _self->history.end(),
            [](const ChatMessage& msg) { return msg.role != "system"; });
        
        if (it != _self->history.end()) {
            _self->history.erase(it);
            approxTokens = estimateTokenCount(_self->history);
        } else {
            break; // Only system messages left
        }
    }
}

// JSON building with RapidJSON
std::string LlamaCppProvider::buildChatCompletionPayload(
    const std::vector<ChatMessage>& history) const {
    
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();
    
    // Model (required by OpenAI-compatible API)
    doc.AddMember("model", "llama.cpp", alloc);
    
    // Sampling parameters
    doc.AddMember("max_tokens", _self->options.max_tokens, alloc);
    doc.AddMember("temperature", _self->options.temperature, alloc);
    doc.AddMember("top_p", _self->options.top_p, alloc);
    doc.AddMember("top_k", _self->options.top_k, alloc);
    doc.AddMember("stream", false, alloc);
    
    // Messages array
    rapidjson::Value messages(rapidjson::kArrayType);
    for (const auto& msg : history) {
        rapidjson::Value entry(rapidjson::kObjectType);
        
        rapidjson::Value roleVal;
        roleVal.SetString(msg.role.c_str(), static_cast<rapidjson::SizeType>(msg.role.length()), alloc);
        entry.AddMember("role", roleVal, alloc);
        
        rapidjson::Value contentVal;
        contentVal.SetString(msg.content.c_str(), static_cast<rapidjson::SizeType>(msg.content.length()), alloc);
        entry.AddMember("content", contentVal, alloc);
        
        messages.PushBack(entry, alloc);
    }
    doc.AddMember("messages", messages, alloc);
    
    // Stop sequences
    if (!_self->options.stop.empty()) {
        rapidjson::Value stopArray(rapidjson::kArrayType);
        for (const auto& stopSeq : _self->options.stop) {
            rapidjson::Value stopVal;
            stopVal.SetString(stopSeq.c_str(), static_cast<rapidjson::SizeType>(stopSeq.length()), alloc);
            stopArray.PushBack(stopVal, alloc);
        }
        doc.AddMember("stop", stopArray, alloc);
    }
    
    // Serialize to string
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    
    return buffer.GetString();
}

std::string LlamaCppProvider::parseAssistantResponse(const std::string& jsonResponse) const {
    rapidjson::Document doc;
    doc.Parse(jsonResponse.c_str());
    
    if (doc.HasParseError()) {
        return "Error: Invalid JSON response";
    }
    
    // Check for API errors
    if (doc.HasMember("error") && doc["error"].IsObject()) {
        const auto& error = doc["error"];
        if (error.HasMember("message") && error["message"].IsString()) {
            return std::string("API Error: ") + error["message"].GetString();
        }
        return "API Error: Unknown error";
    }
    
    // Extract assistant message from OpenAI-compatible format
    // { "choices": [ { "message": { "role":"assistant","content":"..." } } ] }
    if (!doc.HasMember("choices") || !doc["choices"].IsArray() || doc["choices"].Empty()) {
        return "Error: No choices in response";
    }
    
    const auto& choices = doc["choices"];
    const auto& firstChoice = choices[0];
    
    if (!firstChoice.HasMember("message") || !firstChoice["message"].IsObject()) {
        return "Error: No message in choice";
    }
    
    const auto& message = firstChoice["message"];
    if (!message.HasMember("content") || !message["content"].IsString()) {
        return "Error: No content in message";
    }
    
    return message["content"].GetString();
}

} // lab