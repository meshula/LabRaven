//
//  Modes.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 11/26/23.
//  Copyright © 2023. All rights reserved.
//

/*
 Modes has no dependencies, except the the cpp file requires
 moodycamel's concurrentqueue.hpp obtained from
 https://github.com/cameron314/concurrentqueue

 Currently, there is a USD dependency, which can be elided by defining
 HAVE_NO_USD. When I rethink the Transaction object in the future, the
 USD dependency should go away as well as the need for the preprocessor
 directive, of course.
 */

#ifndef Modes_h
#define Modes_h

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
   Activity, or to a major mode. An activity can activate
   an Activaty as a dependency, but cannot activate a major mode.

   Activities can Render to the main viewport, and can run UI,
   which can consist of any number of panels, menus, and what not.

   The hovering and dragging mechanisms allow for an activity to bid for
   owning a hover operation, and a drag operation. The idea behind that
   is that a manipulator might have highest priority for hovering ~ hovering
   over a manipulator would therefore want to "win" versus hovering over a
   model part, or over the sky background.

   A bid of -1 means that the mode does not want to bid for the operation.
 */

// Define ModeActivities in a C-compatible way
typedef struct LabActivity {
    void (*Activate)(void*) = nullptr;
    void (*Deactivate)(void*) = nullptr;
    void (*Update)(void*) = nullptr;
    void (*Render)(void*, const LabViewInteraction*) = nullptr;
    void (*RunUI)(void*, const LabViewInteraction*) = nullptr;
    void (*Menu)(void*) = nullptr;
    void (*ToolBar)(void*) = nullptr;
    int  (*ViewportHoverBid)(void*, const LabViewInteraction*) = nullptr;
    void (*ViewportHovering)(void*, const LabViewInteraction*) = nullptr;
    int  (*ViewportDragBid)(void*, const LabViewInteraction*) = nullptr;
    void (*ViewportDragging)(void*, const LabViewInteraction*) = nullptr;
    const char* name = nullptr; // string is not owned by the activity
    bool active = false;
} LabActivity;

#ifdef __cplusplus
} // extern "C"

namespace lab {
class ModeManager;

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
    
    // removes mode from the journal but does not delete it
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

    friend class ModeManager;

public:
    explicit Activity() {}
    virtual ~Activity() = default;

    virtual const std::string Name() const = 0;

    bool IsActive() const { return activity.active; }

    LabActivity activity;
};


class Mode
{
protected:
    bool _active = false;
    virtual void _activate() {}
    virtual void _deactivate() {}

public:
    explicit Mode() {}
    virtual ~Mode() = default;

    virtual const std::string Name() const = 0;

    virtual void Activate()   final { _active = true;  _activate();   }
    virtual void Deactivate() final { _active = false; _deactivate(); }

    bool IsActive() const { return _active; }
};



/* A Major mode configures the workspace and activates and
   deactivates a set of minor modes */

class MajorMode : public Mode
{
public:
    explicit MajorMode() : Mode() {}
    virtual ~MajorMode() = default;
    virtual const std::vector<std::string>& ModeConfiguration() const = 0;
    virtual bool MustDeactivateUnrelatedModesOnActivation() const { return true; }
};

class ModeManager
{
    struct data;
    data* _self;
    
    Journal _journal;

    std::string _major_mode_pending;

    // private to prevent assignment
    ModeManager& operator=(const ModeManager&);
    
    void _activate_major_mode(const std::string& name);
    void _deactivate_major_mode(const std::string& name);
    void _register_activity(const std::string& name, std::function< std::shared_ptr<Activity>() > fn);
    void _register_major_mode(const std::string& name, std::function< std::shared_ptr<MajorMode>() > fn);

    void _set_activities();

public:
    ModeManager();
    ~ModeManager();
    
    static ModeManager* Canonical();

    const std::map< std::string, std::shared_ptr<Activity> > Activities() const;
    const std::vector<std::string>& ActivityNames() const;
    const std::vector<std::string>& MajorModeNames() const;

    template <typename ActivityType>
    void RegisterActivity(std::function< std::shared_ptr<Activity>() > fn)
    {
        _register_activity(ActivityType::sname(), fn);
    }

    template <typename MajorModeType>
    void RegisterMajorMode(std::function< std::shared_ptr<MajorMode>() > fn)
    {
        static_assert(std::is_base_of<MajorMode, MajorModeType>::value, "must register MajorMode");
        _register_major_mode(MajorModeType::sname(), fn);
    }

    void ActivateMajorMode(const std::string& name) {
        auto m = FindMode(name);
        if (m) {
            _major_mode_pending = name;
        }
    }

    void ActivateActivity(const std::string& name) {
        auto m = FindActivity(name);
        if (m) {
            m->Activate();
            _set_activities();
        }
    }

    void DeactivateActivity(const std::string& name) {
        auto m = FindActivity(name);
        if (m) {
            m->Deactivate();
            _set_activities();
        }
    }

    std::shared_ptr<Mode> FindMode(const std::string &);
    std::shared_ptr<Activity> FindActivity(const std::string &);

    template <typename T>
    std::shared_ptr<T> FindMode()
    {
        auto m = FindMode(T::sname());
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

    MajorMode* CurrentMajorMode() const;

    void RunModeUIs(const LabViewInteraction&);
    void RunViewportHovering(const LabViewInteraction&);
    void RunViewportDragging(const LabViewInteraction&);
    void RunModeRendering(const LabViewInteraction&);
    void RunMainMenu();
        
    void EnqueueTransaction(Transaction&&);
    void UpdateTransactionQueueActivationAndModes();

    Journal& Journal() { return _journal; }
};

} // lab

#endif //__cplusplus
#endif /* Modes_h */
