#include "LlamaCppActivity.hpp"
#include "imgui.h"
#include <chrono>

namespace lab {

LlamaCppActivity::LlamaCppActivity()
    : Activity("LlamaCpp") {
    activity.Update = [](void* instance, float dt) {
        static_cast<LlamaCppActivity*>(instance)->Update(dt);
    };
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<LlamaCppActivity*>(instance)->RunUI(*vi);
    };
}

LlamaCppActivity::~LlamaCppActivity() {
}

void LlamaCppActivity::_activate() {
    // Get Provider from Orchestrator
    provider_ = LlamaCppProvider::instance();
    
    if (!provider_) {
        errorMessage_ = "LlamaCppProvider not available. Ensure it's initialized in Orchestrator.";
        return;
    }
    
    // Clear any existing history
    provider_->clearHistory();
    
    // Apply initial system message
    provider_->addSystemMessage(systemMessageBuffer_);
}

void LlamaCppActivity::_deactivate() {
    provider_ = nullptr;
}

void LlamaCppActivity::Update(float dt) {
    if (!provider_) return;
    
    // Poll for responses from Provider
    std::string response = provider_->pollResponse();
    if (!response.empty()) {
        // Check if it's an error
        if (response.find("Error:") == 0) {
            errorMessage_ = response;
        }
        // Response is already in Provider's history, no need to add
    }
}

void LlamaCppActivity::RunUI(const LabViewInteraction&) {
    ImGui::Begin("LlamaCpp Chat", nullptr, ImGuiWindowFlags_MenuBar);

    // Menu bar
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Options")) {
            if (ImGui::MenuItem("Settings", nullptr, showSettings_)) {
                showSettings_ = !showSettings_;
            }
            if (ImGui::MenuItem("Clear Chat")) {
                clearChat();
            }
            ImGui::Separator();
            ImGui::MenuItem("Auto-scroll", nullptr, &autoScroll_);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    // Settings panel
    if (showSettings_) {
        ImGui::BeginChild("Settings", ImVec2(0, 200), true);
        ImGui::Text("LLM Parameters");
        ImGui::Separator();

        ImGui::InputText("System Message", systemMessageBuffer_, sizeof(systemMessageBuffer_));
        
        ImGui::SliderFloat("Temperature", &temperature_, 0.0f, 2.0f, "%.2f");
        ImGui::SliderFloat("Top P", &topP_, 0.0f, 1.0f, "%.2f");
        ImGui::SliderInt("Top K", &topK_, 1, 100);
        ImGui::SliderInt("Max Tokens", &maxTokens_, 32, 2048);
        ImGui::InputText("Stop Sequence", stopSequence_, sizeof(stopSequence_));

        if (ImGui::Button("Apply Settings", ImVec2(-1, 0))) {
            applySettings();
        }

        ImGui::EndChild();
    }

    // Error display
    if (!errorMessage_.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::TextWrapped("Error: %s", errorMessage_.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine();
        if (ImGui::SmallButton("Clear")) {
            errorMessage_.clear();
        }
    }

    // Chat history
    float inputHeight = 100.0f;
    ImGui::BeginChild("ChatHistory", ImVec2(0, -inputHeight), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    if (provider_) {
        const auto& history = provider_->getHistory();
        for (const auto& msg : history) {
            ImVec4 color;
            const char* prefix;

            if (msg.role == "user") {
                color = ImVec4(0.3f, 0.8f, 0.3f, 1.0f);
                prefix = "You: ";
            } else if (msg.role == "assistant") {
                color = ImVec4(0.3f, 0.6f, 1.0f, 1.0f);
                prefix = "Assistant: ";
            } else {
                color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                prefix = "System: ";
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextWrapped("%s%s", prefix, msg.content.c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }
    }

    if (provider_ && provider_->isProcessing()) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Waiting for response...");
    }

    if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();

    // Input area
    ImGui::BeginChild("InputArea", ImVec2(0, 0), true);

    ImGui::Text("Your message:");

    bool isProcessing = provider_ && provider_->isProcessing();
    ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_CtrlEnterForNewLine;
    if (isProcessing) {
        inputFlags |= ImGuiInputTextFlags_ReadOnly;
    }

    bool enterPressed = ImGui::InputTextMultiline(
        "##input",
        inputBuffer_,
        sizeof(inputBuffer_),
        ImVec2(-1, -30),
        inputFlags
    );

    ImGui::BeginDisabled(isProcessing);
    if (ImGui::Button("Send", ImVec2(80, 0))) {
        sendMessage();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Clear Input", ImVec2(100, 0))) {
        inputBuffer_[0] = '\0';
    }

    ImGui::SameLine();
    ImGui::Text("(Press Enter to send, Ctrl+Enter for new line)");

    ImGui::EndChild();

    ImGui::End();
}

void LlamaCppActivity::sendMessage() {
    if (!provider_) {
        errorMessage_ = "Provider not available";
        return;
    }
    
    if (inputBuffer_[0] == '\0') {
        return;
    }

    std::string userMessage(inputBuffer_);
    inputBuffer_[0] = '\0';
    errorMessage_.clear();

    // Simple delegation to Provider - all complexity handled there!
    provider_->requestChatCompletion(userMessage);
}

void LlamaCppActivity::clearChat() {
    if (provider_) {
        provider_->clearHistory();
        provider_->addSystemMessage(systemMessageBuffer_);
    }
}

void LlamaCppActivity::applySettings() {
    if (!provider_) return;
    
    LlamaCppProvider::Options opts = provider_->getOptions();
    opts.temperature = temperature_;
    opts.top_p = topP_;
    opts.top_k = topK_;
    opts.max_tokens = maxTokens_;
    opts.stop = {std::string(stopSequence_)};
    provider_->setOptions(opts);
    
    // Reapply system message
    provider_->clearHistory();
    provider_->addSystemMessage(systemMessageBuffer_);
}

} // namespace lab