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

class CSP_Process {
    friend class CSP_Engine;
    int id;
public:
    CSP_Process() = delete;
    CSP_Process(const std::string& n, std::function<void()> b)
    : id(-1), name(n), behavior(b) {}

    std::string name;
    std::function<void()> behavior;
};

class CSP_Module {
    friend class CSP_Engine;
    CSP_Engine& engine;
    std::string module_name;
    bool running;

public:
    CSP_Module(CSP_Engine& engine, const std::string& name);
    ~CSP_Module();
    const std::string& get_name() const { return module_name; }

    // must be called to register the module with the engine.
    void Register();

    void emit_event(const CSP_Process& event);

protected:
    void add_process(CSP_Process& proc);
    std::vector<CSP_Process*> processes;
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
    void emit_event(const CSP_Process& event, int msDelay);
    int test();
};


// example

class DemoFileOpenModule : public CSP_Module {
    CSP_Process Ready;
    CSP_Process Opening;
    CSP_Process OpenFile;
    CSP_Process Error;
    CSP_Process Idle;

public:
    DemoFileOpenModule(CSP_Engine& engine)
    : CSP_Module(engine, "DemoFileOpenModule")
    , Ready("Ready", [this]() {
                        std::cout << "Ready: Waiting for file open request...\n";
                        this->emit_event(Opening);
                    })
    , Opening("Opening", [this]() {
                        std::cout << "Opening: File dialog box launched...\n";
                        bool success = true; // Simulate file selection result
                        if (success) {
                            this->emit_event(OpenFile);
                        } else {
                            this->emit_event(Error);
                        }
                    })
    , OpenFile("OpenFile", [this]() {
                        std::cout << "OpenFile: File opened successfully.\n";
                        this->emit_event(Idle);
                    })
    , Error("Error", [this]() {
                        std::cout << "Error: Failed to open file. Returning to Ready...\n";
                        this->emit_event(Idle);
                    })
    , Idle("Idle", [](){})
    {
        add_process(Ready);
        add_process(Opening);
        add_process(OpenFile);
        add_process(Error);
        add_process(Idle);
    }
};


} // lab

#endif // Lab_CSP_hpp
