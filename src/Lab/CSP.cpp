
#include "CSP.hpp"

#include <zmq.hpp>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>

namespace lab {

struct EventData {
    int id;
    char message[];
};

void emit_event(const std::string& event, int id, zmq::socket_t& socket) {
    printf("Emitting %s [%d]\n", event.c_str(), id);
    size_t sz = event.size() + sizeof(int) + 1;
    EventData* event_data = (EventData*) new char[event.size() + sizeof(int) + 1];
    char* messageStr = event_data->message;
    std::memcpy(messageStr, event.data(), event.size());
    messageStr[event.size()] = '\0';
    event_data->id = id;
    zmq::message_t message(event_data, sz);
    socket.send(message, zmq::send_flags::none); // Send the event message
}

CSP_Module::CSP_Module(CSP_Engine& engine, const std::string& name)
: engine(engine)
, module_name(name)
, running(false) {
}

CSP_Module::~CSP_Module() {
    engine.unregister_module(module_name);
}

void CSP_Module::Register() {
    initialize_processes();
    engine.register_module(this);
}

void CSP_Module::add_process(CSP_Process&& proc) {
    processes.emplace_back(proc);
}

void CSP_Module::emit_event(const std::string& event, int id) {
    engine.emit_event(event, id, 0);
}

struct TimedEvent {
    int id; std::string msg; std::chrono::steady_clock::time_point when;

    // implement < for priority queue
    bool operator<(const TimedEvent& other) const {
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

    std::priority_queue<TimedEvent> priorityQueue;
    std::thread serviceThread;
    std::mutex serviceMutex;
    std::condition_variable cv;

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
    , sub_socket(context, ZMQ_SUB), running(false)
    , pub_socket(context, ZMQ_PUB) {
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
        if (self->processes.find(proc.id) != self->processes.end()) {
            std::cerr << "Error: Process id " << proc.id << " already exists in the engine." << std::endl;
        } else {
            self->processes[proc.id] = &proc;
        }
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
        self->processes.erase(proc.id);
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
                self->cv.wait(lock, [this]() { return !self->priorityQueue.empty() || !self->running; });

                // If we're stopping and there are no messages, exit
                if (!self->running && self->priorityQueue.empty()) break;

                // Check the front of the priority queue
                auto now = std::chrono::steady_clock::now();
                auto front = self->priorityQueue.top();

                // If the scheduled time has arrived, send the message
                if (front.when <= now) {
                    std::string message = front.msg;
                    self->priorityQueue.pop();

                    // Send the message via ZMQ
                    /// @TODO need to have the id that was passed in
                    ::lab::emit_event(message, front.id, self->pub_socket);

                    //zmq::message_t message2(5);
                    //memcpy(message2.data(), "Alice", 5);
                    //pub_socket.send(message2, zmq::send_flags::none);

                    std::cout << "Sent event: '" << message << "' at time "
                            << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() << " ms\n";
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

                    EventData* event_data = reinterpret_cast<EventData*>(event_msg.data());
                    int id = event_data->id;
                    std::cout << "Received event [" << id
                                << "] msg: " << event_data->message << std::endl;
                    if (id == 0) continue; // no associated process

                    auto proc = self->processes.find(id);
                    if (proc != self->processes.end()) {
                        proc->second->behavior();
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
void CSP_Engine::emit_event(const std::string& event, int id, int msDelay) {
    // Get current time
    auto now = std::chrono::steady_clock::now();

    // Calculate the future time when the event should be sent
    auto sendTime = now + std::chrono::milliseconds(msDelay);

    {
        std::lock_guard<std::mutex> lock(self->serviceMutex);
        // Insert the message into the priority queue
        self->priorityQueue.push({id, event, sendTime});
    }

    // Notify the service thread that there is a new message to process
    self->cv.notify_one();

    std::cout << "Event '" << event << "' scheduled for: "
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

    emit_event("Event 1.0s", 0, 1000);  // 1 second delay
    emit_event("Event 0.5s", 0, 500);   // 0.5 second delay
    emit_event("Event 2.0s", 0, 2000);  // 2 second delay

    return 0;
}

} // lab
