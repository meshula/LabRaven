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



class CSP_Module;
class CSP_Engine;

struct CSP_Process {
    int id;
    std::string event;
    std::function<void()> behavior;
};

// currently modules can only be singletons. the eventual design is that modules
// can be instanced, and the module is just the program an instance runs.

class CSP_Module {
    CSP_Engine& engine;
    std::string module_name;
    bool running;
    friend class CSP_Engine;

public:
    CSP_Module(CSP_Engine& engine, const std::string& name);
    ~CSP_Module();

    // must be called to register the module with the engine.
    void Register();

    void add_process(CSP_Process&& proc);
    void emit_event(const std::string& event, int id);
    const std::string& get_name() const { return module_name; }

protected:
    virtual void initialize_processes() = 0;
    std::vector<CSP_Process> processes;
};

//#define PUSHPULL

class CSP_Engine {
    struct Self;
    Self* self;

public:
    CSP_Engine();
    ~CSP_Engine();
    zmq::context_t& get_context();

    void register_module(CSP_Module* module);
    void unregister_module(const std::string& module_name);

    void run();
    void stop();

    // Emit an event with a delay (in milliseconds)
     void emit_event(const std::string& event, int id, int msDelay);
    int test();
};


// example

class FileOpenModule : public CSP_Module {
public:
    FileOpenModule(CSP_Engine& engine)
        : CSP_Module(engine, "FileOpenModule") {
    }

private:
    // enum class for Processes, with an underlying int to be used as an id
    enum class Process : int {
        Ready = 100,
        Opening,
        Error,
        OpenFile,
        Idle
    };

    static constexpr int toInt(Process p) { return static_cast<int>(p); }

    virtual void initialize_processes() override {
        add_process({toInt(Process::Ready), "file_open_request",
                     [this]() {
                         std::cout << "Ready: Waiting for file open request...\n";
                         this->emit_event("user_select_file", toInt(Process::Opening));
                     }});

        add_process({toInt(Process::Opening), "user_select_file",
                     [this]() {
                         std::cout << "Opening: File dialog box launched...\n";
                         bool success = true; // Simulate file selection result
                         if (success) {
                             this->emit_event("file_open_success", toInt(Process::OpenFile));
                         } else {
                             this->emit_event("file_open_error", toInt(Process::Error));
                         }
                     }});

        add_process({toInt(Process::Error), "file_open_error",
                    [this]() {
                        std::cout << "Error: Failed to open file. Returning to Ready...\n";
                        this->emit_event("idle", toInt(Process::Idle));
                    }});

        add_process({toInt(Process::OpenFile), "file_open_success",
                    [this]() {
                        std::cout << "OpenFile: File opened successfully.\n";
                        this->emit_event("idle", toInt(Process::Idle));
                    }});

        add_process({toInt(Process::Idle), "reset", [](){} });
    }
};


} // lab

#endif // Lab_CSP_hpp
