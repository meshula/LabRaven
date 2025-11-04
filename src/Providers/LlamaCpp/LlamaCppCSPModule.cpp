#include "LlamaCppCSPModule.hpp"
#include <iostream>
#include <chrono>

namespace lab {

LlamaCppCSPModule::LlamaCppCSPModule(CSP_Engine& engine, LlamaCppProvider* provider)
    : CSP_Module(engine, "LlamaCppModule")
    , provider_(provider)
    , Idle_("Idle", [this]() { this->onIdle(); })
    , SendingRequest_("SendingRequest", [this]() { this->onSendingRequest(); })
    , WaitingForResponse_("WaitingForResponse", [this]() { this->onWaitingForResponse(); })
    , ProcessingResponse_("ProcessingResponse", [this]() { this->onProcessingResponse(); })
    , Error_("Error", [this]() { this->onError(); })
{
    // Register all processes with the CSP engine
    add_process(Idle_);
    add_process(SendingRequest_);
    add_process(WaitingForResponse_);
    add_process(ProcessingResponse_);
    add_process(Error_);
    
    // Start background worker thread
    stopWorker_ = false;
    workerThread_ = std::thread(&LlamaCppCSPModule::workerLoop, this);
    
    // Start in Idle state
    emit_event(Idle_);
}

LlamaCppCSPModule::~LlamaCppCSPModule() {
    stop();
}

void LlamaCppCSPModule::stop() {
    stopWorker_ = true;
    workCV_.notify_all();
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

bool LlamaCppCSPModule::isProcessing() const {
    return processing_.load();
}

void LlamaCppCSPModule::triggerRequest(const std::string& userMessage) {
    if (processing_.load()) {
        std::cerr << "LlamaCppCSPModule: Request already in progress, ignoring new request" << std::endl;
        return;
    }
    
    // Store the user message
    {
        std::lock_guard<std::mutex> lock(workMutex_);
        pendingUserMessage_ = userMessage;
        hasWork_ = true;
    }
    
    // Signal worker thread
    workCV_.notify_one();
    
    // Transition to SendingRequest state
    emit_event(SendingRequest_);
}

// ============================================================================
// State Behaviors
// ============================================================================

void LlamaCppCSPModule::onIdle() {
    // Waiting for work - this state is entered after completing a request
    processing_ = false;
}

void LlamaCppCSPModule::onSendingRequest() {
    // Mark as processing
    processing_ = true;
    
    // The actual request building and sending happens in the worker thread
    // We immediately transition to waiting state
    emit_event(WaitingForResponse_);
}

void LlamaCppCSPModule::onWaitingForResponse() {
    // This state represents waiting for the HTTP response
    // The worker thread will signal when done
    // Poll every 100ms to check if worker completed
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check if worker has completed
    bool workDone = false;
    {
        std::lock_guard<std::mutex> lock(workMutex_);
        workDone = !hasWork_.load();
    }
    
    if (workDone) {
        // Check if we have an error or response
        if (!pendingError_.empty()) {
            emit_event(Error_);
        } else if (!pendingResponse_.empty()) {
            emit_event(ProcessingResponse_);
        }
    } else {
        // Still waiting, loop back to this state
        emit_event(WaitingForResponse_);
    }
}

void LlamaCppCSPModule::onProcessingResponse() {
    // Response received, process it
    std::string response;
void LlamaCppCSPModule::onProcessingResponse() {
    // Response received, process it
    std::string response;
    {
        std::lock_guard<std::mutex> lock(workMutex_);
        response = std::move(pendingResponse_);
        pendingResponse_.clear();
    }
    
    if (!response.empty()) {
        // Push response to provider (thread-safe)
        provider_->pushResponse(response);
    }
    
    // Return to Idle state
    emit_event(Idle_);
}

void LlamaCppCSPModule::onError() {
    // Error occurred, handle it
    std::string error;
    {
        std::lock_guard<std::mutex> lock(workMutex_);
        error = std::move(pendingError_);
        pendingError_.clear();
    }
    
    std::cerr << "LlamaCppCSPModule Error: " << error << std::endl;
    
    // Push error to provider (thread-safe)
    provider_->pushError(error);
    
    // Return to Idle state
    emit_event(Idle_);
}

// ============================================================================
// Worker Thread
// ============================================================================

void LlamaCppCSPModule::workerLoop() {
    while (!stopWorker_.load()) {
        std::unique_lock<std::mutex> lock(workMutex_);
        
        // Wait for work or stop signal
        workCV_.wait(lock, [this] { 
            return hasWork_.load() || stopWorker_.load(); 
        });
        
        if (stopWorker_.load()) {
            break;
        }
        
        if (!hasWork_.load()) {
            continue;
        }
        
        // Get the user message
        std::string userMessage = pendingUserMessage_;
        lock.unlock();
        
        // Get the user message
        std::string userMessage = pendingUserMessage_;
        lock.unlock();
        
        try {
            // Build full history including new user message
            std::vector<ChatMessage> history = provider_->getHistory();
            ChatMessage userMsg;
            userMsg.role = "user";
            userMsg.content = userMessage;
            history.push_back(userMsg);
            
            // Build JSON payload via provider
            std::string payload = provider_->buildChatCompletionPayload(history);
            
            // Create curl client for this request
            CurlClient curl;
            
            // Build endpoint URL
            const auto& opts = provider_->getOptions();
            std::string endpoint = "http://" + opts.host + ":" + 
                                   std::to_string(opts.port) + "/v1/chat/completions";
            
            // Perform HTTP POST
            std::string jsonResponse = curl.post(endpoint, payload, 30);
            
            // Parse response via provider
            std::string assistantMsg = provider_->parseAssistantResponse(jsonResponse);
            
            // Store response
            lock.lock();
            pendingResponse_ = assistantMsg;
            hasWork_ = false;
            lock.unlock();
            
        } catch (const std::exception& e) {
            lock.lock();
            pendingError_ = std::string("HTTP Error: ") + e.what();
            hasWork_ = false;
            lock.unlock();
        }
            lock.unlock();
        }
    }
}

} // lab