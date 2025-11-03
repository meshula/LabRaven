#pragma once

#include "Lab/StudioCore.hpp"
#include <string>
#include <vector>
#include <memory>

namespace lab {

struct ChatMessage {
    std::string role;      // "user" or "assistant"
    std::string content;
    double timestamp;
};

class LlamaCppActivity : public Activity {
protected:
    virtual void _activate() override;
    virtual void _deactivate() override;

public:
    explicit LlamaCppActivity();
    virtual ~LlamaCppActivity();

    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "LlamaCpp"; }

    void RunUI(const LabViewInteraction&);

private:
    // Server configuration
    int port_ = 8080;
    char serverHost_[256] = "127.0.0.1";
    char portBuffer_[16] = "8080";

    // Chat state
    std::vector<ChatMessage> chatHistory_;
    char inputBuffer_[4096] = "";
    bool isWaitingForResponse_ = false;
    std::string errorMessage_;

    // UI state
    bool autoScroll_ = true;
    bool showSettings_ = false;

    // Helper methods
    void sendMessage();
    void clearChat();
    std::string buildServerUrl() const;
    void addMessage(const std::string& role, const std::string& content);
};

} // namespace lab
