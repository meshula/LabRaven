//
//  Modes.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 11/26/23.
//  Copyright © 2023. All rights reserved.
//

#include "Modes.hpp"
#include "concurrentqueue.hpp"
#include <iostream>
#include <set>

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

// removes mode from the journal but does not delete it
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


struct ModeManager::data {
    moodycamel::ConcurrentQueue<Transaction> work_queue;
    MajorMode* current_major_mode = nullptr;
    std::map< std::string, std::shared_ptr<Activity> > minor_modes;
    std::map< std::string, std::shared_ptr<MajorMode> > major_modes;
    std::map< std::string, std::function< std::shared_ptr<Activity>() > > activityFactory;
    std::map< std::string, std::function< std::shared_ptr<MajorMode>() > > majorModeFactory;
    std::vector<std::string> minor_mode_names;
    std::vector<std::string> major_mode_names;
    
    std::vector<Activity*> ui_activities;
    std::vector<Activity*> update_activities;
    std::vector<Activity*> hovering_activities;
    std::vector<Activity*> dragging_activities;
    std::vector<Activity*> rendering_activities;
    std::vector<Activity*> mainmenu_activities;

    void SetActivities() {
        ui_activities.clear();
        update_activities.clear();
        hovering_activities.clear();
        dragging_activities.clear();
        rendering_activities.clear();
        mainmenu_activities.clear();
        for (auto& mm : minor_modes) {
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
            for (auto i : minor_modes) {
                if (i.second->activity.active)
                    std::cout << "  " << i.first << std::endl;
            }
        }
    }
};

namespace {
    ModeManager* gCanonical;
}

ModeManager::ModeManager() {
    _self = new data();
    gCanonical = this;
}

ModeManager::~ModeManager() {
    delete _self;
    gCanonical = nullptr;
}

void ModeManager::_set_activities() {
    _self->SetActivities();
}

//static
ModeManager* ModeManager::Canonical() {
    return gCanonical;
}

MajorMode* ModeManager::CurrentMajorMode() const {
    return _self->current_major_mode;
}

void ModeManager::EnqueueTransaction(Transaction&& work) {
    _self->work_queue.enqueue(work);
}

void ModeManager::UpdateTransactionQueueActivationAndModes() {
    // complete any pending work
    Transaction work;
    while (_self->work_queue.try_dequeue(work)) {
        if (work.exec) {
            std::cout << "> " << work.message << std::endl;
            work.exec();
            _journal.Append(std::move(work));
        }
    }

    // activate any pending major mode
    if (_major_mode_pending.length()) {
        _activate_major_mode(_major_mode_pending);
        _major_mode_pending.clear();
    }
    if (!_self->current_major_mode) {
        _activate_major_mode("Empty");
    }

    for (auto i : _self->update_activities)
        i->activity.Update(i);
}

std::shared_ptr<Mode> ModeManager::FindMode(const std::string & m)
{
    auto maj = _self->major_modes.find(m);
    if (maj != _self->major_modes.end())
        return maj->second;

    auto mmj = _self->majorModeFactory.find(m);
    if (mmj != _self->majorModeFactory.end())
    {
        auto mm = mmj->second();
        _self->major_modes[m] = mm;
        return mm;
    }

    return std::shared_ptr<Mode>();
}

std::shared_ptr<Activity> ModeManager::FindActivity(const std::string & m)
{
    auto mnr = _self->minor_modes.find(m);
    if (mnr != _self->minor_modes.end())
        return mnr->second;

    auto mmn = _self->activityFactory.find(m);
    if (mmn != _self->activityFactory.end())
    {
        auto mm = mmn->second(); // call creation factory
        _self->minor_modes[m] = mm;
        return mm;
    }
    return std::shared_ptr<Activity>();
}

void ModeManager::_activate_major_mode(const std::string& name)
{
    auto m = FindMode(name);
    auto maj = dynamic_cast<MajorMode*>(m.get());
    if (!maj)
        return;

    if (_self->current_major_mode) {
        if (_self->current_major_mode->Name() == name)
            return;
        _deactivate_major_mode(_self->current_major_mode->Name());
    }

    _self->current_major_mode = maj;
    auto modesList = maj->ModeConfiguration();
    std::set<std::string> modes;
    for (auto& i : modesList)
        modes.insert(i);
    
    if (maj->MustDeactivateUnrelatedModesOnActivation()) {
        std::vector<std::string> deactivated;
        
        for (auto minor : _self->minor_modes) {
            if (modes.find(minor.first) == modes.end()) {
                deactivated.push_back(minor.first);
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
    
    for (auto& mode : modes) {
        auto m = FindActivity(mode);
        if (m) {
            std::cout << "Activating " << m->Name() << std::endl;
            m->Activate();
        }
        else {
            std::cerr << "Could not find Activity: " << mode << std::endl;
        }
    }

    maj->Activate();
    _self->SetActivities();
}

void ModeManager::_deactivate_major_mode(const std::string& name)
{
    auto m = FindMode(name);
    auto maj = dynamic_cast<MajorMode*>(m.get());
    if (!maj)
        return;

    if (maj == _self->current_major_mode)
        _self->current_major_mode = nullptr;
    
    auto modes = maj->ModeConfiguration();
    for (auto& mode : modes) {
        auto m = FindActivity(mode);
        if (m) {
            std::cout << "Deactivating " << m->Name() << std::endl;
            m->Deactivate();
        }
        else {
            std::cerr << "Could not find Activity to deactivate: " << mode << std::endl;
        }
    }
    maj->Deactivate();
    _self->SetActivities();
}

void ModeManager::RunModeUIs(const LabViewInteraction& vi) {
    for (auto i : _self->ui_activities)
        if (i->activity.active && i->activity.RunUI)
            i->activity.RunUI(i, &vi);
}

void ModeManager::RunViewportHovering(const LabViewInteraction& vi) {
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

void ModeManager::RunViewportDragging(const LabViewInteraction& vi) {
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

void ModeManager::RunModeRendering(const LabViewInteraction& vi) {
    for (auto i : _self->rendering_activities)
        if (i->activity.active && i->activity.Render)
            i->activity.Render(i, &vi);
}

void ModeManager::RunMainMenu() {
    for (auto i : _self->mainmenu_activities)
        if (i->activity.active && i->activity.Menu)
            i->activity.Menu(i);
}


const std::map< std::string, std::shared_ptr<Activity> > ModeManager::Activities() const {
    return _self->minor_modes;
}

const std::vector<std::string>& ModeManager::ActivityNames() const {
    return _self->minor_mode_names;
}

const std::vector<std::string>& ModeManager::MajorModeNames() const {
    return _self->major_mode_names;
}

void ModeManager::_register_activity(const std::string& name,
                                     std::function< std::shared_ptr<Activity>() > fn) {
    _self->activityFactory[name] = fn;
    _self->minor_mode_names.push_back(name);
}

void ModeManager::_register_major_mode(const std::string& name,
                                       std::function< std::shared_ptr<MajorMode>() > fn) {
    _self->majorModeFactory[name] = fn;
    _self->major_mode_names.push_back(name);
}

} // lab
