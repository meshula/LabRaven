//
//  StudioCore.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 11/26/23.
//  Copyright © 2023. All rights reserved.
//

/*
 StudioCore has no dependencies, except the the cpp file requires
 moodycamel's concurrentqueue.hpp obtained from
 https://github.com/cameron314/concurrentqueue

 Currently, there is a USD dependency, which can be elided by defining
 HAVE_NO_USD. When I rethink the Transaction object in the future, the
 USD dependency should go away as well as the need for the preprocessor
 directive, of course.
 */

/*

Activity
    unique thing to do
Studio
    collection of Activities
Transactions
    Things that change the data in the program 
Journal
    undo/redo history of Transactions
Orchestrator
    Manages the Studios, Activities, Transactions, and Journal

*/

#ifndef StudioCore_h
#define StudioCore_h

#include <stdbool.h>    // for C compatibility
#include <stddef.h>

#ifdef __cplusplus
#include <functional>
#include <map>
#include <string>

#ifndef HAVE_NO_USD
#include <pxr/usd/usd/prim.h>
#endif

extern "C" {
#endif

// C declarations go here

typedef struct LabViewDimensions {
    float w, h;             // view full width and height
    float wx, wy, ww, wh;   // window within the view
} LabViewDimensions;

typedef struct LabViewInteraction {
    LabViewDimensions view;
    float x = 0, y = 0, dt = 0;
    bool start = false, end = false;  // start and end of a drag
} LabViewInteraction;


/* Activities add composable functionality to either another
   Activity, or to a Studio. An activity can activate
   an Activaty as a dependency, but cannot activate a Studio.

   Activities can Render to the main viewport, and can run UI,
   which can consist of any number of panels, menus, and what not.

   The hovering and dragging mechanisms allow for an activity to bid for
   owning a hover operation, and a drag operation. The idea behind that
   is that a manipulator might have highest priority for hovering ~ hovering
   over a manipulator would therefore want to "win" versus hovering over a
   model part, or over the sky background.

   A bid of -1 means that the activity does not want to bid for the operation.
 */

// Define Activities in a C-compatible way
typedef struct LabActivity {
    void* instance = nullptr; // an activity can store its instance data here
    void (*Activate)(void*) = nullptr;
    void (*Deactivate)(void*) = nullptr;
    void (*Update)(void*, float dt) = nullptr;
    void (*Render)(void*, const LabViewInteraction*) = nullptr;
    void (*RunUI)(void*, const LabViewInteraction*) = nullptr;
    void (*Menu)(void*) = nullptr;
    void (*ToolBar)(void*) = nullptr;
    int  (*ViewportHoverBid)(void*, const LabViewInteraction*) = nullptr;
    void (*ViewportHovering)(void*, const LabViewInteraction*) = nullptr;
    int  (*ViewportDragBid)(void*, const LabViewInteraction*) = nullptr;
    void (*ViewportDragging)(void*, const LabViewInteraction*) = nullptr;
    const char* (*Documentation)(void*) = nullptr;
    const char* name = nullptr; // string is not owned by the activity
    bool active = false;
} LabActivity;

typedef struct LabProvider {
    void* instance = nullptr; // a provider can store its instance data here
    const char* (*Documentation)(void*) = nullptr;
    const char* name = nullptr; // string is not owned by the activity
} LabProvider;

#ifdef __cplusplus
} // extern "C"

namespace lab {
class Orchestrator;

struct Transaction {
    std::string message;
    std::function<void()> exec;
    std::function<void()> undo;

#ifndef HAVE_NO_USD
    pxr::UsdPrim prim;
    pxr::TfToken token;
#endif

    Transaction() = default;
    Transaction(std::string m, std::function<void()> e, std::function<void()> u)
        : message(m), exec(e), undo(u) {}
    Transaction(std::string m, std::function<void()> e)
        : message(m), exec(e), undo([](){}) {}

#ifndef HAVE_NO_USD
    Transaction(std::string m, pxr::UsdPrim prim, pxr::TfToken token, std::function<void()> e)
        : message(m), prim(prim), token(token), exec(e), undo([](){}) {}
#endif

    Transaction(Transaction&&) = default;
    Transaction& operator=(Transaction&& t) = default;
    Transaction(const Transaction& t) {
        message = t.message;
        exec = t.exec;
        undo = t.undo;
#ifndef HAVE_NO_USD
        prim = t.prim;
        token = t.token;
#endif
    }
    Transaction& operator=(const Transaction&) = delete;
};

struct JournalNode {
    Transaction transaction;
    JournalNode* next;
    JournalNode* sibling;   // for forking history
    JournalNode* parent;    // for undoing history

    // for debugging, the total extant node count is
    // tracked. The application can call validate() to
    // ensure that the number of nodes equals the count.
    // if it differs, there's a bug in the journal.
    static int count;

    // recursively delete all the nodes after this one
    // making this node the end of the journal
    static void Truncate(JournalNode* node);
    
    JournalNode() : next(nullptr), sibling(nullptr) {
        ++count;
    }
    ~JournalNode() {
        Truncate(this);
        --count;
    }
};

class Journal {
    JournalNode* _curr;

    static void CountHelper(JournalNode* node, int& total);

public:
    Journal();
    bool Validate();
    
    // append a transaction to the journal. If the journal is not at the end,
    // the journal is truncated and the new transaction is appended
    void Append(Transaction&& t);

    // fork the journal, creating a new branch. The current node becomes the
    // sibling of the new branch, and the new branch becomes the current node.
    // If there is already a sibling, the new node becomes a sibling of the
    // current node's sibling, in order that there may be many forks from
    // the same node.
    void Fork(Transaction&& t);
    
    // removes node from the journal but does not delete it
    void Remove(JournalNode* node);
    
    JournalNode root;
};


class Activity
{
protected:
    virtual void _activate() {}
    virtual void _deactivate() {}

    virtual void Activate()   final { activity.active = true;  _activate();   }
    virtual void Deactivate() final { activity.active = false; _deactivate(); }

    friend class Orchestrator;

public:
    explicit Activity() = delete;
    explicit Activity(const char* cname) { activity.name = cname; }
    virtual ~Activity() = default;

    virtual const std::string Name() const = 0;

    bool IsActive() const { return activity.active; }

    LabActivity activity;
};

/* A Studio configures the workspace and activates and
   deactivates a set of activities */

class Studio
{
protected:
    bool _active = false;
    virtual void _activate() {}
    virtual void _deactivate() {}

public:
    explicit Studio() = default;
    virtual ~Studio() = default;

    virtual const std::string Name() const = 0;
    virtual void Activate()   final { _active = true;  _activate();   }
    virtual void Deactivate() final { _active = false; _deactivate(); }

    bool IsActive() const { return _active; }

    virtual const std::vector<std::string>& StudioConfiguration() const = 0;
    virtual bool MustDeactivateUnrelatedActivitiesOnActivation() const { return true; }
};


class Provider
{
public:
    Provider() = delete;
    explicit Provider(const char* cname) { provider.name = cname;}
    virtual ~Provider() = default;
    virtual const std::string Name() const = 0;
    LabProvider provider;
};

class Orchestrator
{
    struct data;
    data* _self;
    
    std::string _studio_pending;
    std::vector<std::pair<std::string, bool>> _activity_pending;

    // private to prevent assignment
    Orchestrator& operator=(const Orchestrator&);
    
    void _activate_studio(const std::string& name);
    void _deactivate_studio(const std::string& name);
    void _register_studio(const std::string& name, std::function< std::shared_ptr<Studio>() > fn);
    void _register_activity(const std::string& name, std::function< std::shared_ptr<Activity>() > fn);
    void _register_provider(const std::string& name, std::function< Provider*() > fn);
    void _set_activities();

public:
    Orchestrator();
    ~Orchestrator();
    
    static Orchestrator* Canonical();

    const std::map< std::string, std::shared_ptr<Activity> > Activities() const;
    const std::vector<std::string>& ActivityNames() const;
    const std::vector<std::string>& StudioNames() const;
    const std::vector<std::string>& ProviderNames() const;

    template <typename ActivityType>
    void RegisterActivity(std::function< std::shared_ptr<Activity>() > fn)
    {
        _register_activity(ActivityType::sname(), fn);
    }

    void RegisterActivity(const std::string& name, std::function< std::shared_ptr<Activity>() > fn)
    {
        _register_activity(name, fn);
    }

    template <typename StudioType>
    void RegisterStudio(std::function< std::shared_ptr<Studio>() > fn)
    {
        static_assert(std::is_base_of<Studio, StudioType>::value, "must register Studio");
        _register_studio(StudioType::sname(), fn);
    }

    template <typename ProviderType>
    void RegisterProvider(std::function< Provider* () > fn)
    {
        static_assert(std::is_base_of<Provider, ProviderType>::value, "must register Provider");
        _register_provider(ProviderType::sname(), fn);
    }

    void ActivateStudio(const std::string& name) {
        auto m = FindStudio(name);
        if (m) {
            _studio_pending = name;
        }
    }

    void ActivateActivity(const std::string& name) {
        auto m = FindActivity(name);
        if (m) {
            _activity_pending.push_back({name, true});
        }
    }

    void DeactivateActivity(const std::string& name) {
        auto m = FindActivity(name);
        if (m) {
            _activity_pending.push_back({name, false});
        }
    }

    std::shared_ptr<Studio> FindStudio(const std::string &);
    std::shared_ptr<Activity> FindActivity(const std::string &);
    Provider* FindProvider(const std::string &);

    template <typename T>
    std::shared_ptr<T> FindStudio()
    {
        auto m = FindStudio(T::sname());
        return std::dynamic_pointer_cast<T>(m);
    }

    template <typename T>
    std::shared_ptr<T> LockActivity(std::weak_ptr<T>& m) {
        auto r = m.lock();
        if (!r) {
            auto activity = FindActivity(T::sname());
            if (activity) {
                m = std::dynamic_pointer_cast<T>(activity);
                r = std::dynamic_pointer_cast<T>(m.lock());
            }
        }
        return r;
    }

    Studio* CurrentStudio() const;

    void RunActivityUIs(const LabViewInteraction&);
    void RunViewportHovering(const LabViewInteraction&);
    void RunViewportDragging(const LabViewInteraction&);
    void RunActivityRendering(const LabViewInteraction&);
    void RunMainMenu();
        
    void EnqueueTransaction(Transaction&&);
    void ServiceTransactionsAndActivities(float dt);

    Journal& GetJournal() const;
};

} // lab

#endif //__cplusplus
#endif /* StudioCore_h */
