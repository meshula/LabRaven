#ifndef Lab_CSP_hpp
#define Lab_CSP_hpp

#include <zmq.hpp>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>

namespace lab {

class CSP_Engine {
public:
    CSP_Engine() : context(1), socket(context, ZMQ_ROUTER), running(false) {
        socket.bind("inproc://csp_engine");
    }

    ~CSP_Engine() { stop(); }

    void register_module(const std::string& module_name, zmq::socket_t& module_socket) {
        std::lock_guard<std::mutex> lock(mutex);
        modules[module_name] = &module_socket;
    }

    void unregister_module(const std::string& module_name) {
        std::lock_guard<std::mutex> lock(mutex);
        modules.erase(module_name);
    }

    void run() {
        running = true;
        worker = std::thread(&CSP_Engine::process_events, this);
    }

    void stop() {
        if (running) {
            running = false;
            socket.send(zmq::message_t("STOP", 4), zmq::send_flags::none); // Graceful shutdown
            worker.join();
        }
    }

private:
    zmq::context_t context;
    zmq::socket_t socket;
    std::map<std::string, zmq::socket_t*> modules;
    std::thread worker;
    std::mutex mutex;
    bool running;

    void process_events() {
        while (running) {
            zmq::message_t module_name_msg, event_msg;

            // Receive module name and event
            socket.recv(module_name_msg, zmq::recv_flags::none);
            socket.recv(event_msg, zmq::recv_flags::none);

            std::string module_name(static_cast<char*>(module_name_msg.data()), module_name_msg.size());
            std::string event(static_cast<char*>(event_msg.data()), event_msg.size());

            std::lock_guard<std::mutex> lock(mutex);
            if (modules.find(module_name) != modules.end()) {
                modules[module_name]->send(event_msg, zmq::send_flags::none); // Forward to module
            }
        }
    }
};

class CSP_Module;
struct CSP_Process {
    std::string name;
    std::string event;
    std::function<void(class CSP_Module&, int)> behavior;
};

class CSP_Module {
public:
    CSP_Module(CSP_Engine& engine, const std::string& name)
        : engine(engine), module_name(name), context(1), socket(context, ZMQ_PAIR), running(false) {
        socket.connect("inproc://csp_engine");
        engine.register_module(module_name, socket);
    }

    ~CSP_Module() {
        engine.unregister_module(module_name);
    }

    void emit_event(const std::string& event, int id) {
        zmq::message_t message(event.size() + sizeof(int));
        std::memcpy(message.data(), event.data(), event.size());
        std::memcpy(static_cast<char*>(message.data()) + event.size(), &id, sizeof(int));
        socket.send(message, zmq::send_flags::none);
    }

protected:
    std::vector<CSP_Process> processes;

private:
    CSP_Engine& engine;
    std::string module_name;
    zmq::context_t context;
    zmq::socket_t socket;
    std::thread worker;
    bool running;
};

// example

class FileOpenModule : public CSP_Module {
public:
    FileOpenModule(CSP_Engine& engine)
        : CSP_Module(engine, "FileOpenModule") {
        initialize_states();
    }

private:
    void initialize_states() {
        processes.push_back({"Ready", "file_open_request",
                             [](CSP_Module& module, int) {
                                 std::cout << "Ready: Waiting for file open request...\n";
                                 module.emit_event("user_select_file", 0);
                             }});

        processes.push_back({"Opening", "user_select_file",
                             [](CSP_Module& module, int) {
                                 std::cout << "Opening: File dialog box launched...\n";
                                 bool success = true; // Simulate file selection result
                                 module.emit_event(success ? "file_open_success" : "file_open_error", 0);
                             }});

        processes.push_back({"Error", "file_open_error",
                             [](CSP_Module& module, int) {
                                 std::cout << "Error: Failed to open file. Returning to Ready...\n";
                                 module.emit_event("reset", 0);
                             }});

        processes.push_back({"OpenFile", "file_open_success",
                             [](CSP_Module& module, int) {
                                 std::cout << "OpenFile: File opened successfully.\n";
                                 module.emit_event("reset", 0);
                             }});

        processes.push_back({"Reset", "reset", [](CSP_Module&, int) {}});
    }
};


} // lab

#endif // Lab_CSP_hpp
