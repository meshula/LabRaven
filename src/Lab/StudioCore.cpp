//
//  StudioCore.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 11/26/23.
//  Copyright © 2023. All rights reserved.
//

#include "StudioCore.hpp"

#include <iostream>
#include <set>
#include <thread>
#include <zmq.hpp>

namespace lab
{
using namespace std;


// static
int JournalNode::count = 0;

// recursively delete all the nodes after this one
// making this node the end of the journal
void JournalNode::Truncate(JournalNode* node) {
    if (!node)
        return;
    if (node->next) {
        Truncate(node->next);
        delete node->next;
        node->next = nullptr;
    }
    if (node->sibling) {
        Truncate(node->sibling);
        delete node->sibling;
        node->sibling = nullptr;
    }
}

void Journal::CountHelper(JournalNode* node, int& total) {
    // recurse the next list, and the sibling lists to count the nodes
    if (node->next)
        CountHelper(node->next, total);
    if (node->sibling)
        CountHelper(node->sibling, total);
    ++total;
}

Journal::Journal() : _curr(&root) {
    root.transaction.undo = [](){
        throw std::runtime_error("Cannot undo journal root"); };
    root.transaction.message = "Session start";
}

bool Journal::Validate() {
    int total = 0;
    CountHelper(&root, total);
    return total == JournalNode::count;
}

// append a transaction to the journal. If the journal is not at the end,
// the journal is truncated and the new transaction is appended
void Journal::Append(Transaction&& t) {
    // if _curr->next is not null, we are not at the end of the journal
    if (_curr->next) {
        JournalNode::Truncate(_curr->next);
        delete _curr->next;
    }
    
#ifndef HAVE_NO_USD
    if (!t.token.IsEmpty() && (t.prim == _curr->transaction.prim && t.token == _curr->transaction.token)) {
        // if the transaction's prim & token match, overwrite the current
        // journal node. This is so that things like interactively dragging a manipulator
        // accumulate only a single node.
        _curr->transaction = std::move(t);
    }
    else 
#endif
    {
        _curr->next = new JournalNode();
        _curr->next->parent = _curr;
        _curr->next->transaction = std::move(t);
        _curr = _curr->next;
    }
}

// fork the journal, creating a new branch. The current node becomes the
// sibling of the new branch, and the new branch becomes the current node.
// If there is already a sibling, the new node becomes a sibling of the
// current node's sibling, in order that there may be many forks from
// the same node.
void Journal::Fork(Transaction&& t) {
    // if the current node has a sibling, follow the siblings until we
    // find the last one, and set that to _curr.
    while (_curr->sibling)
        _curr = _curr->sibling;
    _curr->sibling = new JournalNode();
    _curr->sibling->parent = _curr->parent;
    _curr->sibling->transaction = std::move(t);
    _curr = _curr->sibling;
}

// removes node from the journal but does not delete it
void Journal::Remove(JournalNode* node) {
    if (!node)
        return;

    JournalNode::Truncate(node);

    // if node is in the sibling list of the parent, remove it
    if (node->parent) {
        JournalNode* prev = nullptr;
        JournalNode* curr = node->parent->sibling;
        while (curr) {
            if (curr == node) {
                if (prev)
                    prev->sibling = curr->sibling;
                else
                    node->parent->sibling = curr->sibling;
                break;
            }
            prev = curr;
            curr = curr->sibling;
        }
    }

    // if node is next of the parent, remove it
    if (node->parent && node->parent->next == node)
        node->parent->next = nullptr;

    // if node is the current node, move to the parent
    if (_curr == node)
        _curr = node->parent;
}



struct Orchestrator::data {
    Studio* current_studio = nullptr;
    std::map< std::string, std::shared_ptr<Activity> > activities;
    std::map< std::string, std::shared_ptr<Studio> > studios;
    std::map< std::string, Provider* > providers;
    std::map< std::string, std::function< std::shared_ptr<Activity>() > > activityFactory;
    std::map< std::string, std::function< std::shared_ptr<Studio>() > > studioFactory;
    std::map< std::string, std::function< Provider*() > > providerFactory;
    std::vector<std::string> activity_names;
    std::vector<std::string> studio_names;
    std::vector<std::string> provider_names;
    
    std::vector<Activity*> ui_activities;
    std::vector<Activity*> update_activities;
    std::vector<Activity*> hovering_activities;
    std::vector<Activity*> dragging_activities;
    std::vector<Activity*> rendering_activities;
    std::vector<Activity*> mainmenu_activities;

    Journal journal;

    struct Transactor {
        zmq::context_t context;
        zmq::socket_t pull_socket;
        zmq::socket_t push_socket;
        std::thread processing_thread;
        Transactor()
            : context(1) // one thread, because low frequency and in process.
            , pull_socket(context, ZMQ_PULL)
            , push_socket(context, ZMQ_PUSH) {
                pull_socket.bind("inproc://transactions");
                push_socket.connect("inproc://transactions");
            }
    } transactor;

    data() {
        transactor.processing_thread = std::thread([this]() {
            while (true) {
                zmq::message_t message;

                if (transactor.pull_socket.recv(message, zmq::recv_flags::none)) {

                    // Placement delete and copy to local scope
                    Transaction* work = static_cast<Transaction*>(message.data());
                    Transaction transaction = std::move(*work);
                    work->~Transaction();

                    // Handle special termination logic if needed
                    if (transaction.message == "STOP") {
                        break;
                    }

                    // Process transaction
                    if (transaction.exec) {
                        std::cout << "> " << transaction.message << std::endl;
                        transaction.exec();
                        journal.Append(std::move(transaction));
                    }
                }
                else  {
                    // there was an issue. @TODO handle or report it.
                }
            }
        });
    }

    ~data() {
        zmq::socket_t stop_socket(transactor.context, ZMQ_PUSH);
        stop_socket.connect("inproc://transactions");
        Transaction stop_transaction("STOP", []() {});
        zmq::message_t message(sizeof(Transaction));
        new (message.data()) Transaction(std::move(stop_transaction)); // Placement new
        stop_socket.send(message, zmq::send_flags::none);
        transactor.processing_thread.join(); // Wait for the thread to exit
    }

    void SetActivities() {
        ui_activities.clear();
        update_activities.clear();
        hovering_activities.clear();
        dragging_activities.clear();
        rendering_activities.clear();
        mainmenu_activities.clear();
        for (auto& mm : activities) {
            if (mm.second->activity.active) {
                Activity* a = mm.second.get();
                if (a->activity.RunUI) ui_activities.push_back(a);
                if (a->activity.Update) update_activities.push_back(a);
                if (a->activity.ViewportHoverBid) hovering_activities.push_back(a);
                if (a->activity.ViewportDragBid) dragging_activities.push_back(a);
                if (a->activity.Render) rendering_activities.push_back(a);
                if (a->activity.Menu) mainmenu_activities.push_back(a);
            }
        }
        static bool noisy = true;
        if (noisy) {
            std::cout << "Active Activities: \n";
            for (auto i : activities) {
                if (i.second->activity.active)
                    std::cout << "  " << i.first << std::endl;
            }
        }
    }
};

namespace {
    Orchestrator* gCanonical;
}

Orchestrator::Orchestrator() {
    _self = new data();
    gCanonical = this;
}

Orchestrator::~Orchestrator() {
    delete _self;
    gCanonical = nullptr;
}

void Orchestrator::_set_activities() {
    _self->SetActivities();
}

//static
Orchestrator* Orchestrator::Canonical() {
    return gCanonical;
}

Journal& Orchestrator::GetJournal() const {
    return _self->journal;
}

Studio* Orchestrator::CurrentStudio() const {
    return _self->current_studio;
}

void Orchestrator::EnqueueTransaction(Transaction&& work) {
    zmq::message_t message(sizeof(Transaction));
    new (message.data()) Transaction(std::move(work)); // Placement new
    _self->transactor.push_socket.send(message, zmq::send_flags::none);
}

void Orchestrator::ServiceTransactionsAndActivities(float dt) {
    // activate pending studio if ready
    if (_studio_pending.length()) {
        _activate_studio(_studio_pending);
        _studio_pending.clear();
    }
    if (!_self->current_studio) {
        _activate_studio("Empty");
    }

    if (_activity_pending.size()) {
        for (auto& i : _activity_pending) {
            auto m = FindActivity(i.first);
            if (m) {
                if (i.second)
                    m->Activate();
                else
                    m->Deactivate();
            }
        }
        _activity_pending.clear();
        _set_activities();
    }

    for (auto i : _self->update_activities)
        i->activity.Update(i, dt);
}

std::shared_ptr<Studio> Orchestrator::FindStudio(const std::string & m)
{
    auto maj = _self->studios.find(m);
    if (maj != _self->studios.end())
        return maj->second;

    auto mmj = _self->studioFactory.find(m);
    if (mmj != _self->studioFactory.end())
    {
        auto mm = mmj->second();
        _self->studios[m] = mm;
        return mm;
    }

    return std::shared_ptr<Studio>();
}

std::shared_ptr<Activity> Orchestrator::FindActivity(const std::string & m)
{
    auto mnr = _self->activities.find(m);
    if (mnr != _self->activities.end())
        return mnr->second;

    auto mmn = _self->activityFactory.find(m);
    if (mmn != _self->activityFactory.end())
    {
        auto mm = mmn->second(); // call creation factory
        _self->activities[m] = mm;
        return mm;
    }
    return std::shared_ptr<Activity>();
}

Provider* Orchestrator::FindProvider(const std::string & m)
{
    auto mnr = _self->providers.find(m);
    if (mnr != _self->providers.end())
        return mnr->second;

    auto mmn = _self->providerFactory.find(m);
    if (mmn != _self->providerFactory.end())
    {
        auto mm = mmn->second(); // call creation factory
        _self->providers[m] = mm;
        return mm;
    }
    return nullptr;
}

void Orchestrator::_activate_studio(const std::string& name)
{
    auto m = FindStudio(name);
    auto maj = dynamic_cast<Studio*>(m.get());
    if (!maj)
        return;

    if (_self->current_studio) {
        if (_self->current_studio->Name() == name)
            return;
        _deactivate_studio(_self->current_studio->Name());
    }

    _self->current_studio = maj;
    auto activitiesList = maj->StudioConfiguration();
    std::set<std::string> activities;
    for (auto& i : activitiesList)
        activities.insert(i);

    if (maj->MustDeactivateUnrelatedActivitiesOnActivation()) {
        std::vector<std::string> deactivated;
        
        for (auto activity : _self->activities) {
            if (activities.find(activity.first) == activities.end()) {
                deactivated.push_back(activity.first);
            }
        }
        
        for (auto i : deactivated) {
            auto m = FindActivity(i);
            if (m) {
                std::cout << "Deactivating" << m->Name() << std::endl;
                m->Deactivate();
            }
        }
    }
    
    for (auto& activity : activities) {
        auto m = FindActivity(activity);
        if (m) {
            std::cout << "Activating " << m->Name() << std::endl;
            m->Activate();
        }
        else {
            std::cerr << "Could not find Activity: " << activity << std::endl;
        }
    }

    maj->Activate();
    _self->SetActivities();
}

void Orchestrator::_deactivate_studio(const std::string& name)
{
    auto m = FindStudio(name);
    auto maj = dynamic_cast<Studio*>(m.get());
    if (!maj)
        return;

    if (maj == _self->current_studio)
        _self->current_studio = nullptr;
    
    auto activities = maj->StudioConfiguration();
    for (auto& activity : activities) {
        auto m = FindActivity(activity);
        if (m) {
            std::cout << "Deactivating " << m->Name() << std::endl;
            m->Deactivate();
        }
        else {
            std::cerr << "Could not find Activity to deactivate: " << activity << std::endl;
        }
    }
    maj->Deactivate();
    _self->SetActivities();
}

void Orchestrator::RunActivityUIs(const LabViewInteraction& vi) {
    for (auto i : _self->ui_activities)
        if (i->activity.active && i->activity.RunUI)
            i->activity.RunUI(i, &vi);
}

void Orchestrator::RunViewportHovering(const LabViewInteraction& vi) {
    Activity* dragger = nullptr;
    int highest_bidder = -1;

    for (auto i : _self->hovering_activities)
        if (i->activity.active && i->activity.ViewportHoverBid) {
            int bid = i->activity.ViewportHoverBid(i, &vi);
            if (bid > highest_bidder) {
                dragger = i;
                highest_bidder = bid;
            }
        }

    if (dragger) {
        dragger->activity.ViewportHovering(dragger, &vi);
    }
}

void Orchestrator::RunViewportDragging(const LabViewInteraction& vi) {
    Activity* dragger = nullptr;
    int highest_bidder = -1;

    for (auto i : _self->dragging_activities)
        if (i->activity.active && i->activity.ViewportDragBid) {
            int bid = i->activity.ViewportDragBid(i, &vi);
            if (bid > highest_bidder) {
                dragger = i;
                highest_bidder = bid;
            }
        }

    if (dragger) {
        dragger->activity.ViewportDragBid(dragger, &vi);
    }
}

void Orchestrator::RunActivityRendering(const LabViewInteraction& vi) {
    for (auto i : _self->rendering_activities)
        if (i->activity.active && i->activity.Render)
            i->activity.Render(i, &vi);
}

void Orchestrator::RunMainMenu() {
    for (auto i : _self->mainmenu_activities)
        if (i->activity.active && i->activity.Menu)
            i->activity.Menu(i);
}

const std::map< std::string, std::shared_ptr<Activity> > Orchestrator::Activities() const {
    return _self->activities;
}

const std::vector<std::string>& Orchestrator::ActivityNames() const {
    return _self->activity_names;
}

const std::vector<std::string>& Orchestrator::StudioNames() const {
    return _self->studio_names;
}

const std::vector<std::string>& Orchestrator::ProviderNames() const {
    return _self->provider_names;
}

void Orchestrator::_register_activity(const std::string& name,
                                     std::function< std::shared_ptr<Activity>() > fn) {
    _self->activityFactory[name] = fn;
    _self->activity_names.push_back(name);
}

void Orchestrator::_register_studio(const std::string& name,
                                       std::function< std::shared_ptr<Studio>() > fn) {
    _self->studioFactory[name] = fn;
    _self->studio_names.push_back(name);
}

void Orchestrator::_register_provider(const std::string& name,
                                     std::function< Provider*() > fn) {
    _self->providerFactory[name] = fn;
    _self->provider_names.push_back(name);
}

} // lab
