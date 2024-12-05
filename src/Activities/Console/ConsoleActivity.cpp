//
//  ConsoleActivity.cpp
//  LabExcelsior
//
//  Created by Nick Porcino on 12/22/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#include "ConsoleActivity.hpp"
#include "imgui_console/imgui_console.h"
#include "Lab/Landru.hpp"
#include "Lab/LabDirectories.h"
#include <mutex>
#include <thread>
#include <zmq.hpp>


csys::ItemLog& operator << (csys::ItemLog &log, ImVec4& vec)
{
    log << "ImVec4: [" << vec.x << ", "
                       << vec.y << ", "
                       << vec.z << ", "
                       << vec.w << "]";
    return log;
}

static void imvec4_setter(ImVec4 & my_type, std::vector<int> vec)
{
    if (vec.size() < 4) return;

    my_type.x = vec[0]/255.f;
    my_type.y = vec[1]/255.f;
    my_type.z = vec[2]/255.f;
    my_type.w = vec[3]/255.f;
}

namespace lab {

struct LogMessage {
    csys::ItemType type;  // Log level or type
    std::string message;   // The actual log message

    LogMessage(csys::ItemType t, std::string_view msg)
        : type(t), message(msg) {}

    // Serialize the LogMessage into a zmq::message_t
    zmq::message_t to_zmq_message() const {
        // Allocate space for both type and message
        zmq::message_t msg(sizeof(type) + message.size());
        auto data = msg.data();

        // Serialize the ItemType and message
        memcpy(data, &type, sizeof(type));
        memcpy(static_cast<char*>(data) + sizeof(type), message.data(), message.size());
        return msg;
    }

    // Deserialize a zmq::message_t into a LogMessage
    static LogMessage from_zmq_message(const zmq::message_t& msg) {
        auto data = msg.data();
        csys::ItemType t = *static_cast<const csys::ItemType*>(data);
        std::string_view msg_view(static_cast<const char*>(data) + sizeof(csys::ItemType), msg.size() - sizeof(csys::ItemType));
        return LogMessage(t, msg_view);
    }
};


struct ConsoleActivity::data {

    struct ConsoleSubscriber {
        zmq::context_t context;
        zmq::socket_t subscriber;
        zmq::socket_t publisher; // for sending a STOP message
        std::thread subscriber_thread;
        ImGuiConsole& console;

        ConsoleSubscriber(ImGuiConsole& console)
        : context(1)
        , publisher(context, ZMQ_PUB)
        , subscriber(context, ZMQ_SUB)
        , console(console) {
            subscriber.connect("inproc://console");
            publisher.connect("inproc://console");
            subscriber.set(zmq::sockopt::subscribe, ""); // "" means subscribe to all messages

            subscriber_thread = std::thread([this](){
                while (true) {
                    zmq::message_t message;
                    subscriber.recv(&message);
                    // break if STOP is received
                    if (message.size() == 4 && memcmp(message.data(), "STOP", 4) == 0)
                        break;

                    // Deserialize the message
                    LogMessage log_msg = LogMessage::from_zmq_message(message);

                    // Log the message
                    this->console.System().Log(log_msg.type) << log_msg.message << csys::endl;
                }
            });
        }
        ~ConsoleSubscriber() {
            // send a STOP message to the subscriber thread
            zmq::message_t message(4);
            memcpy(message.data(), "STOP", 4);
            publisher.send(message);
            subscriber_thread.join();
        }
    } consoleSubscriber;

    data() : console("Console"), consoleSubscriber(console) {}
    ~data() {}

    void init() {
        // Register variables
        console.System().RegisterVariable("background_color", clear_color, imvec4_setter);

        // Register scripts
        try {
            //console.System().RegisterScript("test_script", "./console.script");
        }
        catch(...) {
            std::cerr << "console.script not loaded\n";
        }
        
        // Register custom commands
        console.System().RegisterCommand(
                "random_background_color", 
                "Assigns a random color to the background application",
                [&clear_color = this->clear_color]() {
                    clear_color.x = (rand() % 256) / 256.f;
                    clear_color.y = (rand() % 256) / 256.f;
                    clear_color.z = (rand() % 256) / 256.f;
                    clear_color.w = (rand() % 256) / 256.f;
                });
        console.System().RegisterCommand(
                "reset_background_color", 
                "Reset background color to its original value",
                [&clear_color = this->clear_color, val = clear_color]() {
                    clear_color = val;
                });
        console.System().RegisterCommand(
                "dirs",
                "List directories",
                [this]() {
                    console.System().Log(csys::ItemType::INFO)
                        << "Directories:" << csys::endl;
                    console.System().Log(csys::ItemType::INFO) << "exectuable:  "
                        << lab_application_executable_path(nullptr) << csys::endl;
                    console.System().Log(csys::ItemType::INFO) << "resources:   "
                        << lab_application_resource_path(nullptr, nullptr) << csys::endl;
                    console.System().Log(csys::ItemType::INFO) << "preferences: "
                        << lab_application_preferences_path(nullptr) << csys::endl;
                });

        console.System().RegisterCommand("l", "Run landru program",
                                         [this](const csys::String &landru) {
            try {
                LandruRun(landru.m_String.c_str());
            }
            catch(std::exception& e) {
                console.System().Log(csys::ItemType::ERROR) << "Couldn't run the landru program" << csys::endl;

                console.System().Log(csys::ItemType::ERROR) << e.what() << csys::endl;
            }
        }, csys::Arg<csys::String>("landru"));
    }

    ImGuiConsole console;
    ImVec4 clear_color = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    std::once_flag must_init;
    bool ui_visible = true;
};

ConsoleActivity::ConsoleActivity()
: Activity(ConsoleActivity::sname())
, _self(new data)
{
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<ConsoleActivity*>(instance)->RunUI(*vi);
    };
}

ConsoleActivity::~ConsoleActivity()
{
    delete _self;
}

void ConsoleActivity::RunUI(const LabViewInteraction&)
{
    if (!_self->ui_visible)
        return;
    
    std::call_once(_self->must_init, [this]() {
        _self->init();
    });
    _self->console.Draw();
}

void ConsoleActivity::Log(std::string_view s) {
    LogMessage msg(csys::ItemType::LOG, s);
    _self->consoleSubscriber.publisher.send(msg.to_zmq_message());
}

void ConsoleActivity::Warning(std::string_view s) {
    LogMessage msg(csys::ItemType::WARNING, s);
    _self->consoleSubscriber.publisher.send(msg.to_zmq_message());
}

void ConsoleActivity::Error(std::string_view s) {
    LogMessage msg(csys::ItemType::ERROR, s);
    _self->consoleSubscriber.publisher.send(msg.to_zmq_message());
}

void ConsoleActivity::Info(std::string_view s) {
    LogMessage msg(csys::ItemType::INFO, s);
    _self->consoleSubscriber.publisher.send(msg.to_zmq_message());
}


} // lab
