
#include "CSP.hpp"

#include <zmq.hpp>
#include <atomic>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>

namespace lab {


CSP_Module::CSP_Module(CSP_Engine& engine, const std::string& name)
: engine(engine)
, module_name(name)
, running(false) {
}

CSP_Module::~CSP_Module() {
    engine.unregister_module(module_name);
}

void CSP_Module::Register() {
    engine.register_module(this);
}

void CSP_Module::add_process(CSP_Process& proc) {
    processes.emplace_back(&proc);
}

void CSP_Module::emit_event(const CSP_Process& proc) {
    engine.emit_event(proc, 0);
}

struct TimedProcess {
    const CSP_Process* process;
    std::chrono::steady_clock::time_point when;

    // implement < for priority queue
    bool operator<(const TimedProcess& other) const {
        return when > other.when;
    }
};

struct CSP_Engine::Self {
    zmq::context_t context;
    zmq::socket_t pub_socket;
    zmq::socket_t sub_socket;
    std::thread worker;
    std::mutex workerMutex;
    bool running;

    std::map<int, CSP_Process*> processes;
    std::map<std::string, CSP_Module*> modules;

    std::priority_queue<TimedProcess> timedProcessQueue;
    std::thread serviceThread;
    std::mutex serviceMutex;
    std::condition_variable cv;

    std::atomic<int> nextProcess{1};
    std::atomic<int> nextModule{1};
    struct EventData {
        int id;
        std::string message;
        const CSP_Process* process;
    };

    std::map<int, EventData> pendingProcesses;

    void AsyncSendProcess(const CSP_Process& process, zmq::socket_t& socket) {
        int pendingMessageId = ++nextProcess;
        pendingProcesses[pendingMessageId] = {pendingMessageId, process.name, &process};

        // package pendingMessageId and process.name into a message and send it on a socket
        // structure it as name:id
        std::string event = process.name + ":" + std::to_string(pendingMessageId);
        printf("Sending event: %s\n", event.c_str());

        zmq::message_t message(event.size());
        memcpy(message.data(), event.data(), event.size());
        socket.send(message, zmq::send_flags::none);
    }
    

#ifdef PUSHPULL
    Self()
    : context(1)
    , sub_socket(context, ZMQ_PULL), running(false)
    , pub_socket(context, ZMQ_PUSH) {
        pub_socket.connect("inproc://csp_engine");
        sub_socket.bind("inproc://csp_engine");
        //sub_socket.set(zmq::sockopt::subscribe, ""); // "" means subscribe to all messages
    }
#else
    Self()
    : context(1)
    , sub_socket(context, ZMQ_SUB)
    , running(false)
    , pub_socket(context, ZMQ_PUB)
    , nextProcess(1) {
        pub_socket.bind("inproc://csp_engine");
        sub_socket.connect("inproc://csp_engine");
        sub_socket.set(zmq::sockopt::subscribe, ""); // "" means subscribe to all messages
    }
#endif
};

CSP_Engine::CSP_Engine() : self(new Self()) {
}

CSP_Engine::~CSP_Engine() {
    stop();
    delete self;
}

void CSP_Engine::register_module(CSP_Module* module) {
    std::lock_guard<std::mutex> lock(self->workerMutex);
    // if the module is already registered, return
    if (self->modules.find(module->get_name()) != self->modules.end()) {
        std::cerr << "Error: Module " << module->get_name() << " already exists in the engine." << std::endl;
        return;
    }
    self->modules[module->get_name()] = module;

    // add all the module's process ids to the processes map
    // report an error if a process id is already in the map.
    for (auto& proc : module->processes) {
        proc->id = self->nextProcess++;
        self->processes[proc->id] = proc;
    }
}

void CSP_Engine::unregister_module(const std::string& module_name) {
    std::lock_guard<std::mutex> lock(self->workerMutex);
    auto module = self->modules.find(module_name);
    if (module == self->modules.end()) {
        return;
    }
    // remove all the module's process ids from the processes map
    for (auto& proc : module->second->processes) {
        self->processes.erase(proc->id);
    }
    self->modules.erase(module_name);
}

void CSP_Engine::run() {
    if (!self->running) {
        self->running = true;
        self->serviceThread = std::thread([this]() {
            // service the timed priority queue
            while (self->running) {
                std::unique_lock<std::mutex> lock(self->serviceMutex);

                // Wait until there are messages in the queue or the thread is stopped
                self->cv.wait(lock, [this]() { return !self->timedProcessQueue.empty() || !self->running; });

                // If we're stopping and there are no messages, exit
                if (!self->running && self->timedProcessQueue.empty()) break;

                // Check the front of the priority queue
                auto now = std::chrono::steady_clock::now();
                auto front = self->timedProcessQueue.top();

                // If the scheduled time has arrived, send the message
                if (front.when <= now) {
                    const CSP_Process* process = front.process;
                    self->timedProcessQueue.pop();

                    // Send the message via ZMQ
                    /// @TODO need to have the id that was passed in
                    self->AsyncSendProcess(*process, self->pub_socket);

                    //zmq::message_t message2(5);
                    //memcpy(message2.data(), "Alice", 5);
                    //pub_socket.send(message2, zmq::send_flags::none);
                }
                else {
                    // Otherwise, if the front message isn't due yet, wait until it is
                    self->cv.wait_until(lock, front.when);
                }
            }
        });

        self->worker = std::thread([this]() {
            std::cout << "Waiting for inproc://csp_engine events" << std::endl;
            while (self->running) {
                zmq::message_t event_msg;
                std::optional<size_t> rcvd_bytes = self->sub_socket.recv(event_msg, zmq::recv_flags::none);
                if (rcvd_bytes > 0) {
                    std::string event(static_cast<char*>(event_msg.data()), event_msg.size());
                    if (event == "STOP") break;
                    if (event == "Alice" || event == "Bob") {
                        std::cout << "Received " << event << std::endl;
                        continue;
                    }

                    const char* msg = reinterpret_cast<const char*>(event_msg.data());
                    if (!msg) {
                        continue;
                    }
                    int id = 0;
                    std::string message(msg);
                    auto colon = message.find(':');
                    if (colon != std::string::npos) {
                        id = std::stoi(message.substr(colon + 1));
                        message = message.substr(0, colon);
                    }

                    std::cout << "Received event [" << id
                              << "] msg: " << message << std::endl;
                    if (id == 0)
                        continue; // no associated process

                    auto proc = self->pendingProcesses.find(id);
                    if (proc != self->pendingProcesses.end()) {
                        proc->second.process->behavior();
                    }
                    else {
                        std::cerr << "Error: Process id " << id << " not found in the engine." << std::endl;
                    }
                }
            }
        });
    }
}

void CSP_Engine::stop() {
    if (self->running) {
        self->running = false;
        self->cv.notify_all();  // Wake up the timed thread in case it is waiting
        self->serviceThread.join();

        zmq::message_t stop_msg("STOP", 4);
        self->pub_socket.send(stop_msg, zmq::send_flags::none); // Graceful shutdown
        self->worker.join();
    }
}

// Emit an event with a delay (in milliseconds)
void CSP_Engine::emit_event(const CSP_Process& process, int msDelay) {
    // Get current time
    auto now = std::chrono::steady_clock::now();

    // Calculate the future time when the event should be sent
    auto sendTime = now + std::chrono::milliseconds(msDelay);

    {
        std::lock_guard<std::mutex> lock(self->serviceMutex);
        // Insert the message into the priority queue
        self->timedProcessQueue.push({&process, sendTime});
    }

    // Notify the service thread that there is a new message to process
    self->cv.notify_one();

    std::cout << "Event '" << process.name << "' scheduled for: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(sendTime.time_since_epoch()).count() << " ms\n";
}

zmq::context_t& CSP_Engine::get_context() { return self->context; }

int CSP_Engine::test() {
    run();
    zmq::message_t message(5);
    memcpy(message.data(), "Alice", 5);
    self->pub_socket.send(message, zmq::send_flags::none);
    zmq::message_t message2(3);
    memcpy(message2.data(), "Bob", 3);
    self->pub_socket.send(message2, zmq::send_flags::none);

    static CSP_Process e0("Event 1.0s", [](){ std::cout << "Event 1.0s"; });
    static CSP_Process e1("Event 0.5s", [](){ std::cout << "Event 0.5s"; });
    static CSP_Process e2("Event 2.0s", [](){ std::cout << "Event 2.0s"; });

    emit_event(e0, 1000);  // 1 second delay
    emit_event(e1,  500);  // 0.5 second delay
    emit_event(e2, 2000);  // 2 second delay

    return 0;
}

} // lab
