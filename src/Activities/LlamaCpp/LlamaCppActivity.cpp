#include "LlamaCppActivity.hpp"
#include "imgui.h"
#include <chrono>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
#endif

namespace lab {

LlamaCppActivity::LlamaCppActivity()
    : Activity("LlamaCpp") {
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<LlamaCppActivity*>(instance)->RunUI(*vi);
    };
}

LlamaCppActivity::~LlamaCppActivity() {
}

void LlamaCppActivity::_activate() {
    // Add welcome message
    addMessage("system", "LlamaCpp chat interface initialized. Configure the server settings and start chatting!");
}

void LlamaCppActivity::_deactivate() {
    chatHistory_.clear();
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
        ImGui::BeginChild("Settings", ImVec2(0, 100), true);
        ImGui::Text("Server Configuration");
        ImGui::Separator();

        ImGui::InputText("Host", serverHost_, sizeof(serverHost_));

        if (ImGui::InputText("Port", portBuffer_, sizeof(portBuffer_), ImGuiInputTextFlags_CharsDecimal)) {
            port_ = std::atoi(portBuffer_);
        }

        ImGui::Text("Server URL: %s", buildServerUrl().c_str());

        if (ImGui::Button("Test Connection")) {
            errorMessage_ = "Connection test not yet implemented";
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
    float inputHeight = 80.0f;
    ImGui::BeginChild("ChatHistory", ImVec2(0, -inputHeight), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    for (const auto& msg : chatHistory_) {
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

    if (isWaitingForResponse_) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Waiting for response...");
    }

    if (autoScroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();

    // Input area
    ImGui::BeginChild("InputArea", ImVec2(0, 0), true);

    ImGui::Text("Your message:");

    ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_CtrlEnterForNewLine;
    if (isWaitingForResponse_) {
        inputFlags |= ImGuiInputTextFlags_ReadOnly;
    }

    bool enterPressed = ImGui::InputTextMultiline(
        "##input",
        inputBuffer_,
        sizeof(inputBuffer_),
        ImVec2(-1, -30),
        inputFlags
    );

    if (ImGui::Button("Send", ImVec2(80, 0))) {
        sendMessage();
    }

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
    if (inputBuffer_[0] == '\0') {
        return;
    }

    std::string userMessage(inputBuffer_);
    addMessage("user", userMessage);
    inputBuffer_[0] = '\0';

    isWaitingForResponse_ = true;
    errorMessage_.clear();

    // Build the request
    std::string url = buildServerUrl() + "/completion";

    // Build JSON payload
    std::stringstream jsonPayload;
    jsonPayload << "{\n";
    jsonPayload << "  \"prompt\": \"";

    // Escape the message for JSON
    for (char c : userMessage) {
        if (c == '"' || c == '\\') {
            jsonPayload << '\\';
        }
        jsonPayload << c;
    }

    jsonPayload << "\",\n";
    jsonPayload << "  \"n_predict\": 512,\n";
    jsonPayload << "  \"temperature\": 0.7,\n";
    jsonPayload << "  \"stop\": [\"</s>\", \"User:\"],\n";
    jsonPayload << "  \"stream\": false\n";
    jsonPayload << "}";

    std::string payload = jsonPayload.str();

    // Simple HTTP POST request
    try {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

        // Create socket
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        // Resolve hostname
        struct hostent* server = gethostbyname(serverHost_);
        if (server == nullptr) {
            throw std::runtime_error("Failed to resolve hostname");
        }

        // Set up server address
        struct sockaddr_in serverAddr;
        memset(&serverAddr, 0, sizeof(serverAddr));
        serverAddr.sin_family = AF_INET;
        memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);
        serverAddr.sin_port = htons(port_);

        // Connect
        if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
#ifdef _WIN32
            closesocket(sockfd);
            WSACleanup();
#else
            close(sockfd);
#endif
            throw std::runtime_error("Failed to connect to server");
        }

        // Build HTTP request
        std::stringstream request;
        request << "POST /completion HTTP/1.1\r\n";
        request << "Host: " << serverHost_ << ":" << port_ << "\r\n";
        request << "Content-Type: application/json\r\n";
        request << "Content-Length: " << payload.length() << "\r\n";
        request << "Connection: close\r\n";
        request << "\r\n";
        request << payload;

        std::string requestStr = request.str();

        // Send request
        send(sockfd, requestStr.c_str(), requestStr.length(), 0);

        // Receive response
        std::string response;
        char buffer[4096];
        int bytesReceived;

        while ((bytesReceived = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0) {
            buffer[bytesReceived] = '\0';
            response += buffer;
        }

        // Close socket
#ifdef _WIN32
        closesocket(sockfd);
        WSACleanup();
#else
        close(sockfd);
#endif

        // Parse response (very simple parsing)
        size_t bodyStart = response.find("\r\n\r\n");
        if (bodyStart != std::string::npos) {
            std::string body = response.substr(bodyStart + 4);

            // Extract content from JSON response (simple parsing)
            size_t contentPos = body.find("\"content\":");
            if (contentPos != std::string::npos) {
                size_t startQuote = body.find('"', contentPos + 10);
                size_t endQuote = body.find('"', startQuote + 1);

                if (startQuote != std::string::npos && endQuote != std::string::npos) {
                    std::string assistantMessage = body.substr(startQuote + 1, endQuote - startQuote - 1);

                    // Unescape JSON
                    std::string unescaped;
                    for (size_t i = 0; i < assistantMessage.length(); ++i) {
                        if (assistantMessage[i] == '\\' && i + 1 < assistantMessage.length()) {
                            ++i;
                            if (assistantMessage[i] == 'n') {
                                unescaped += '\n';
                            } else {
                                unescaped += assistantMessage[i];
                            }
                        } else {
                            unescaped += assistantMessage[i];
                        }
                    }

                    addMessage("assistant", unescaped);
                } else {
                    addMessage("assistant", body);
                }
            } else {
                errorMessage_ = "Failed to parse response";
            }
        } else {
            errorMessage_ = "Invalid HTTP response";
        }

    } catch (const std::exception& e) {
        errorMessage_ = std::string("Error: ") + e.what();
    }

    isWaitingForResponse_ = false;
}

void LlamaCppActivity::clearChat() {
    chatHistory_.clear();
    addMessage("system", "Chat cleared");
}

std::string LlamaCppActivity::buildServerUrl() const {
    std::stringstream ss;
    ss << "http://" << serverHost_ << ":" << port_;
    return ss.str();
}

void LlamaCppActivity::addMessage(const std::string& role, const std::string& content) {
    ChatMessage msg;
    msg.role = role;
    msg.content = content;

    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    msg.timestamp = std::chrono::duration<double>(duration).count();

    chatHistory_.push_back(msg);
}

} // namespace lab
