#ifndef Providers_LlamaCpp_CSPModule_hpp
#define Providers_LlamaCpp_CSPModule_hpp

#include "Lab/CSP.hpp"
#include "LlamaCppProvider.hpp"
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace lab {

/**
 * CSP State Machine for asynchronous llama.cpp server communication.
 * 
 * State Flow:
 * Idle → SendingRequest → WaitingForResponse → ProcessingResponse → Idle
 *                               ↓
 *                            Error → Idle
 * 
 * This module runs the HTTP request in a background thread while
 * the state machine coordinates via ZeroMQ CSP events.
 */
class LlamaCppCSPModule : public CSP_Module {
public:
    LlamaCppCSPModule(CSP_Engine& engine, LlamaCppProvider* provider);
    ~LlamaCppCSPModule();
    
    /**
     * Trigger a chat completion request.
     * Called by Provider when user sends a message.
     * @param userMessage The user's input
     */
    void triggerRequest(const std::string& userMessage);
    
    /**
     * Check if module is currently processing.
     */
    bool isProcessing() const;
    
    /**
     * Stop the background worker thread.
     */
    void stop();
    
private:
    // CSP States
    CSP_Process Idle_;
    CSP_Process SendingRequest_;
    CSP_Process WaitingForResponse_;
    CSP_Process ProcessingResponse_;
    CSP_Process Error_;
    
    // Provider reference (not owned)
    LlamaCppProvider* provider_;
    
    // Request data
    std::string pendingUserMessage_;
    std::string pendingResponse_;
    std::string pendingError_;
    
    // Background worker
    std::thread workerThread_;
    std::atomic<bool> stopWorker_{false};
    std::atomic<bool> hasWork_{false};
    std::atomic<bool> processing_{false};
    
    // Thread synchronization
    std::mutex workMutex_;
    std::condition_variable workCV_;
    
    // State behaviors
    void onIdle();
    void onSendingRequest();
    void onWaitingForResponse();
    void onProcessingResponse();
    void onError();
    
    // Worker thread function
    void workerLoop();
};

} // lab

#endif // Providers_LlamaCpp_CSPModule_hpp