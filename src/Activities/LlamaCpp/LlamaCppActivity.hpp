#pragma once

#include "Lab/StudioCore.hpp"
#include "Providers/LlamaCpp/LlamaCppProvider.hpp"
#include <string>
#include <vector>

namespace lab {

class LlamaCppActivity : public Activity {
protected:
    virtual void _activate() override;
    virtual void _deactivate() override;

public:
    explicit LlamaCppActivity();
    virtual ~LlamaCppActivity();

    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "LlamaCpp"; }

    void Update(float dt);
    void RunUI(const LabViewInteraction&);

private:
    // Provider access (via Orchestrator)
    LlamaCppProvider* provider_ = nullptr;
    
    // UI state
    char inputBuffer_[4096] = "";
    bool autoScroll_ = true;
    bool showSettings_ = false;
    std::string errorMessage_;
    
    // Options UI
    char systemMessageBuffer_[1024] = "You are a helpful assistant.";
    float temperature_ = 0.3f;
    float topP_ = 0.9f;
    int topK_ = 40;
    int maxTokens_ = 256;
    char stopSequence_[64] = "</s>";
    
    // Helper methods
    void sendMessage();
    void clearChat();
    void applySettings();
};

} // namespace lab