
#include "LabText.h"
#include <stdio.h>

/*
 **Landru** is an agent oriented Lisp-like. Previously, it was
 Forth-like.
 
 Comments are **C++** style.

 ```
 // This is a comment.
 ```

 **Landru** allows the import of modules using the *require* keyword.

 ```
(require audio)
(require time)
 ```
 **Landru** has a number of standard libraries, including
  - io -- input and output
  - time -- time related utilities
  - int -- integer math
  - real -- real number math

 There are some optional libraries as well, such as
  - audio

 The fundamental execution unit is a *machine*. When machines are
 encountered in a Landru file, they are compiled. There is one special
 machine, named *main*. It's special because it is executed automatically as
 soon as the whole file is compiled.

 ```
 // this machine is trivial
(machine main)
 ```

 Machines are made up of declared persistent state, and states. There is one
 special state, named *main*. When a machine is started, the machine's state
 variables are instantiated, and then *main* automatically runs.

 ```
(
    (require audio)
    (machine main
        (set audioContext (audio.Context create)
        (state main)
    )
)
 ```
Whitespace is not significant.

 ```
((require audio) (machine main (set audioContext (audio.Context create)
(state main)))
 ```

Functions work as is familiar from Lisp.
 ```
    (int.add 3 5)   // result is discarded
    (set sum (int.add 3 5))
 ```


 A *machine* can launch another *machine*.

 ```
 (machine pong
   (state main (io.print "pong")))

 (machine main
   (state main (io.print("ping")
               (launch poing)))
 ```

 A *machine* can launch another instance of its own kind.

 ```
 (machine recurse
    (state main (launch recurse)))
 ```

 The *recurse* machine does not lock up the execution environment,
 because *lauch* instructions do not execute immediately. Instead, they are
 queued, and run when a state yields.

 A *machine* retires when it has no pending state transitions. **Landru**
 execution continues until all launched machines retire, or are otherwise
 terminated.

 A real *machine* will have more than one state. State transitions are invoked
 by the *goto* statement.

 ```
 (machine pingpong
   (state ping (io.print("ping") (goto pong))
   (state pong (io.print("pong") (goto ping))
   (state main (goto ping)))
 ```
 
 
 The *pingpong machine* will print out ping and pong like crazy. (Not completely
 crazy, gotos yield before they take affect.) Actor based
 languages are event driven, and **Landru** is no exception. **Landru** uses
 the *on* keyword to indicate events that will be responded to.

 The *time* standard library has an after event. The *pingpong machine* can be
 brought under control by involving a timer.

 ```
 (
 (machine main
     (state ping
         (io.print "ping!")
         (on time.after 0.5
             (goto pong)
         )
     )

     (state pong
         (io.print "pong!")
         (on time.after 0.5
             (goto ping)
         )
     )
     (state main
         (goto ping)
     )
 )
 (launch main)
 )
 ```

 This *pingpong machine* will change state every half a second. Notice that
 the *on* statement takes the form of *on condition* followed by the things to do
 as a series of lists.

 variables
 ---------

 Variables have a hierarchy of scopes.

 Local variables are declared within a scope, and don't persist beyond it.

 ```
 (machine localVar
   (state main
     (set a 3.0)
     (io.print `("a is " a)))
 ```
 
 Note the Lisp-like backtick, indicating that a list is merely a list.

 The variable *a* won't be avalable outside of the state, and will be initialized
 to 3 every time the state is initialized.

 ```
(machine instanceVar
   (set b 3.0)
   (state main
     (io.print `("b is " b)))
 ```

 When a machine of type *instanceVar* is launched, it will have its own copy of *b*,
 initialized to 3. If a state modifies b, the new value is seen by any other states
 within that instatiated machine, but not by other machines of the same time.

 ```
 (machine sharedInstanceVar
   (shared c 3.0)
   (state main
     (io.print `("c is " c)))
 ```

 When the first machine of type *sharedInstanceVar* is instantiated, *c* will be
 have the value of 3. If *c* is modified, all other instances of *sharedInstanceVar*
 will see that new value. When new instances are created, they will share the
 already existing *c*. If all copies of *sharedInstanceVar* exit, *c* will also
 disappear, meaning the next *sharedINstanceVar* object will reinitialize *c* with
 the value 3.

 ```
 (require test)
 (machine requireVar
   (state main
     (io.print `("test.d is " d)))
 ```

 If the module test is known to have a value *d* in it, a machine can access it
 using dotted scope syntax.

 ```
(set e 3.0)

 (machine globalVar
   (state main
     (io.print `("global e is " e)))
 ```

 Variables declared in the global scope persist until all launched machines have
 exited and the script stops running.

 States can have local variables.

 ```
(machine stateVar
     (state main
       (set j 5)))
 ```

 Scoping rules are that the most local scope has precedence. So a local variable
 is preferred to machine variable (shared or instance), is preferred to a required
 module's variable or a global variable. If for some reason namespacing caused a
 global variable to have the same name as a module's variable, the module
 variable hides the global variable.
 
 */


#include <map>
using std::map;
#include <string>
using std::string;
#include <vector>
using std::vector;

char const* pingpongSrc = R"(

(require time)
(require io)

(io.print "hello world!")

(machine main
    (state ping
        (io.print "ping!")
        (on time.after 0.5
            (goto pong)
        )
    )

    (state pong
        (io.print "pong!")
        (on time.after 0.5
            (goto ping)
        )
    )
    (state main
        (goto ping)
    )
)
(launch main)

)";


//----------------------------------------------------------------------

// forward declarations
struct Landru;
struct MachineExemplar;
struct MachineInstance;
//----------------------------------------------------------------------

// function pointer for library functions whose signature is
// void func(Landru* l, tsParsedSexpr_t* curr, MachineInstance* inst);
typedef void (*LandruLibFunc)(Landru*, tsParsedSexpr_t*, MachineInstance*);
//----------------------------------------------------------------------

typedef enum {
    moLaunchMachine,
    moGotoState,
    moSetVariable,
    moOn,
    moFunctionCall,
} MachineOperation;

typedef struct {
    MachineOperation op;
    tsParsedSexpr_t* arg; // for launch, goto, args for call
    LandruLibFunc func;   // for call function
    tsStrView_t name;     // for debugging purposes
    MachineExemplar* me;  // for when the operation needs to recurse
} MachineInstruction;

typedef enum {
    mtRoot,
    mtMachine,
    mtState,
    mtOn,
    mtFunction,
    mtMonad,
    mtList,
} MachineType;

typedef struct MachineExemplar {
    string name;
    MachineType isState;
    MachineExemplar* parent;
    tsParsedSexpr_t* arg;
    vector<MachineInstruction> instructions;
    map<string, MachineExemplar*> machines;
    map<string, MachineExemplar*> functions;
    vector<MachineExemplar*> ons;
} MachineExemplar;

//----------------------------------------------------------------------
// A machine instance contains the local runtime scope for a machine.
// THe local scope context contains the machine exemplar, the local variables, 
// and the callbacks and functions defined in the machine.
// The lcoal context also contains all the states that can be dispatched
// from the current context.
// The instance contains not only the local runtime scope, but also all the
// child scopes that have been launched from this scope.

typedef struct {
    tsParsedSexpr_t* arguments;
    map<string, tsParsedSexpr_t*> vars;
    map<string, tsParsedSexpr_t*> callbacks;
    map<string, tsParsedSexpr_t*> functions;
    map<string, tsParsedSexpr_t*> states;
} MachineLocalContext;

typedef struct MachineInstance {
    MachineExemplar* me;
    vector<MachineLocalContext> localStates;
} MachineInstance;

//----------------------------------------------------------------------
// The Landru context contains the all of the library functions
// imported by the program. It contains the root machine instance
// which comprises the global scope.
//
// It contains a list of the imported libraries, for debugging purposes,
// and a list of the machines to be launched the next time the engine
// is updated. That list is cleared after the machines are launched.

typedef struct {
    MachineExemplar* me; // machine to launch
    MachineInstance* parent; // the scope to launch the machine into
} PendingLaunch;

typedef struct Landru {
    MachineInstance root;
    vector<string> req;
    map<string, LandruLibFunc> libraryFunctions;
    vector<PendingLaunch> launch;
} Landru;

//----------------------------------------------------------------------

tsParsedSexpr_t* compileMachine(
    tsParsedSexpr_t* curr, 
    Landru* l, 
    MachineType mt, 
    MachineExemplar** exemplar);

//----------------------------------------------------------------------

void print_spaces(int indent) {
    static char spaces[256];
    static int spaces_sz = 0;
    if (spaces_sz == 0) {
        for (int i = 0; i < 255; ++i) {
            spaces[i] = ' ';
        }
        spaces[255] = 0;
        spaces_sz = 255;
    }
    if (indent < 0 || indent > 255)
        return;

    printf("%s", spaces + (255 - indent));
}

typedef enum {
    markerAtom, markerClose, markerReturn
} printMarker;

void dump_parsed(tsParsedSexpr_t* curr) {
    int indent = 0;
    printMarker marker = markerAtom;

    while (curr) {
        switch (curr->token) {
        case tsSexprAtom: {
            std::string s = std::string(curr->str.curr, curr->str.sz);
            printf("%s ", s.c_str());
            marker = markerAtom;
            break;
        }
        case tsSexprPushList:
            printf("\n"); print_spaces(indent); printf("(");
            indent += 3;
            marker = markerAtom;
            break;
        case tsSexprPopList:
            indent -= 3;
            if (marker == markerReturn)
                print_spaces(indent);
            printf(")\n");
            marker = markerReturn;
            break;
        case tsSexprInteger:
            printf(" %lld ", curr->i);
            marker = markerAtom;
            break;
        case tsSexprFloat:
            printf(" %f ", curr->f);
            marker = markerAtom;
            break;
        case tsSexprString: {
            std::string s = std::string(curr->str.curr, curr->str.sz);
            printf(" \"%s\" ", s.c_str());
            marker = markerAtom;
            break;
        }
        }
        curr = curr->next;
    } // while
    printf("\n");
}


void dumpExemplar(MachineExemplar* me, int indent = 0) {
    if (!me)
        return;

    print_spaces(indent);
    printf("MachineExemplar: %s", me->name.c_str());
    switch (me->isState) {
        case mtRoot:     printf("  type: root\n"); break;
        case mtMachine:  printf("  type: machine\n"); break;
        case mtState:    printf("  type: state\n"); break;
        case mtOn:       printf("  type: on\n"); break;
        case mtFunction: printf("  type: function\n"); break;
        case mtMonad:    printf("  type: monad\n"); break;
        case mtList:     printf("  type: list\n"); break;
    }
    for (auto& instr : me->instructions) {
        print_spaces(indent);
        switch (instr.op) {
            case moLaunchMachine: printf("  op: launch machine"); break;
            case moGotoState: printf("  op: goto state"); break;
            case moSetVariable: printf("  op: set variable"); break;
            case moOn: printf("  op: on"); break;
            case moFunctionCall: printf("  op: function call"); break;
        }
        printf("  [%.*s]\n", (int) instr.name.sz, instr.name.curr);
    }
    for (auto& on : me->ons) {
        print_spaces(indent);
        printf("  on: %s\n", on->name.c_str());
        dumpExemplar(on, indent + 2);
    }
    for (auto& func : me->functions) {
        print_spaces(indent);
        printf("  function: %s\n", func.second->name.c_str());
        dumpExemplar(func.second, indent + 2);
    }
    for (auto& machine : me->machines) {
        dumpExemplar(machine.second, indent + 2);
    }
}

void dumpMachineInstance(MachineInstance* inst) {
    if (!inst)
        return;
    
    printf("MachineInstance: %s\n", inst->me->name.c_str());
    printf("  states:\n");
    for (auto i = inst->localStates.back().states.begin(); i != inst->localStates.back().states.end(); ++i) {
        printf("    %s\n", i->first.c_str());
    }
}

void dumpLandru(Landru* l) {
    printf("require:\n");
    for (auto& s : l->req) {
        printf("  %s\n", s.c_str());
    }
}

void dumpParsedExpr(const char* prefix, tsParsedSexpr_t* curr) {
    if (curr) {
        switch(curr->token) {
            case tsSexprAtom:
                printf("%s[atom] %.*s\n", prefix, (int) curr->str.sz, curr->str.curr);
                break;
            case tsSexprFloat:
                printf("%s%f\n", prefix, curr->f);
                break;
            case tsSexprInteger:
                printf("%s%lld\n", prefix, curr->i);
                break;
            case tsSexprString:
                printf("%s[string] %.*s\n", prefix, (int) curr->str.sz, curr->str.curr);
                break;
            case tsSexprPushList:
                printf("%spush list\n", prefix);
                break;
            case tsSexprPopList:
                printf("%spop list\n", prefix);
                break;
        }
    }
}

[[noreturn]] void diagnoseAndThrow(tsParsedSexpr_t* curr, const char* msg) {
    printf("Error: %s\n Current token and following are:\n", msg);
    for (int i = 0; i < 7; ++i) {
        if (curr) {
            dumpParsedExpr(i == 0? "--> " : "    ", curr);
            curr = curr->next;
        }
    }
    
    throw std::runtime_error(msg);
}

//----------------------------------------------------------------------
// lib io
//

void io_register(Landru* l) {
}

void registerLibrary(Landru* l, const char* name) {
    if (strcmp(name, "io") == 0) {
        io_register(l);
    }
    else if (strcmp(name, "time") == 0) {
        //
    }
    else if (strcmp(name, "test") == 0) {
        //
    }
    else
        throw std::runtime_error("unknown library");
}

//----------------------------------------------------------------------

bool atomIsKeyword(const tsParsedSexpr_t* curr) {
    if (!curr || curr->token != tsSexprAtom)
        return false;

    static const char* keywords[] = {"require", "machine", "state", "on", "goto", "launch"};
    static const int numKeywords = sizeof(keywords) / sizeof(keywords[0]);
    for (int i = 0; i < numKeywords; ++i) {
        if (sizeof(keywords[i]) != curr->str.sz)
            continue;
        if (strncmp(curr->str.curr, keywords[i], curr->str.sz) == 0)
            return true;
    }
    return false;
}

void landru_add_require(Landru* l, tsParsedSexpr_t* curr) {
    if (!curr || curr->token != tsSexprAtom || !curr->str.sz)
        return;

    l->req.push_back(std::string(curr->str.curr, curr->str.sz));
}

LandruLibFunc findLibraryFunction(Landru* l, const char* name) {    // search root
    auto f = l->libraryFunctions.find(name);
    if (f != l->libraryFunctions.end())
        return f->second;
    return NULL;
}

tsParsedSexpr_t* findFunction(Landru* l, MachineInstance* inst, const char* name) {
    // search local states
    for (auto it = inst->localStates.rbegin(); it != inst->localStates.rend(); ++it) {
        auto& ctx = *it;
        auto f = ctx.functions.find(name);
        if (f != ctx.functions.end()) {
            return f->second;
        }
    }
    // search root
    auto f = l->root.localStates[0].functions.find(name);
    if (f != l->root.localStates[0].functions.end()) {
        return f->second;
    }
    return NULL;
}


tsParsedSexpr_t* findClosingParen(tsParsedSexpr_t* openParen) {
    int level = 0;
    tsParsedSexpr_t* curr = openParen;
    while (curr && level >= 0) {
        switch (curr->token) {
            case tsSexprPushList:
                level++;
                break;
            case tsSexprPopList:
                level--;
                if (level == 0)
                    return curr;
                break;
            case tsSexprAtom:
            case tsSexprString:
            case tsSexprFloat:
            case tsSexprInteger:
                break;
        }
        curr = curr->next;
    }
    return NULL;
}

void signalStateExit(MachineInstance* inst) {
    // if we have a current state, then call the state exit function
    // and clear the state's local context. The current context becomes
    // the state's context's parent context.
    if (!inst || !inst->localStates.size())
        throw std::runtime_error("no current state");

    inst->localStates.pop_back();
}

void signalStateEnter(MachineInstance* inst) {
    // if we have a current state, then call the state enter function
    // and create a new local context. Set the state's local context's parent
    // to the current context, and set the current context to the new
    // local context.

    if (!inst)
        throw std::runtime_error("no current state");

    inst->localStates.push_back(MachineLocalContext{});
}

void signalCallbackCancelled(MachineInstance* inst) {
    // if we have a current state, then call the callback cancelled function
    // and clear the state's local context. The current context becomes
    // the state's context's parent context.
    if (!inst || !inst->localStates.size())
        throw std::runtime_error("no current state");
}

void signalCallbackSet(MachineInstance* inst) {
    // if we have a current state, then call the callback set function
    // and create a new local context. Set the state's local context's parent
    // to the current context, and set the current context to the new
    // local context.

    if (!inst)
        throw std::runtime_error("no current state");
}

void diagnoseStateNotFound(MachineInstance* inst, string stateName) {
    printf("state not found: %s\n", stateName.c_str());
    printf("available states:\n");
    for (auto& c : inst->localStates) {
        for (auto& s : c.states) {
            printf("  %s\n", s.first.c_str());
        }
    }
}

tsParsedSexpr_t* compileLaunch(tsParsedSexpr_t* curr, MachineExemplar* me) {
    // peek at next to get the name of the machine to launch
    tsParsedSexpr_t* name = curr->next;
    if (name && name->token == tsSexprAtom) {
        std::string nameStr = std::string(name->str.curr, name->str.sz);
        
        // the next should be a close paren
        tsParsedSexpr_t* body = name->next;
        if (!body || body->token != tsSexprPopList)
            diagnoseAndThrow(curr, "expected end of list");
        
        MachineInstruction instr;
        instr.op = moLaunchMachine;
        instr.name = name->str;
        me->instructions.push_back(instr);
        return body; // return closing paren
    }
    else {
        diagnoseAndThrow(curr, "expected machine name");
    }
}

tsParsedSexpr_t* compileGoto(tsParsedSexpr_t* curr, MachineExemplar* me) {
    // peek at next to get the name of the state to launch
    tsParsedSexpr_t* name = curr->next;
    if (name && name->token == tsSexprAtom) {
        std::string nameStr = std::string(name->str.curr, name->str.sz);
        
        // the next should be a close paren
        tsParsedSexpr_t* body = name->next;
        if (!body || body->token != tsSexprPopList)
            diagnoseAndThrow(curr, "expected end of list");
        
        MachineInstruction instr;
        instr.op = moGotoState;
        instr.name = name->str;
        me->instructions.push_back(instr);
        return body; // return closing paren
    }
    else {
        diagnoseAndThrow(curr, "expected state name");
    }
}

tsParsedSexpr_t* compileSet(tsParsedSexpr_t* curr, Landru* l, MachineExemplar* me) {
    // peek at next to get the name of variable to set
    tsParsedSexpr_t* name = curr->next;
    if (name && name->token == tsSexprAtom) {
        std::string nameStr = std::string(name->str.curr, name->str.sz);
        
        // the next token should either be a constant or a list
        tsParsedSexpr_t* body = name->next;
        switch (body->token) {
            case tsSexprPushList: {
                // the next token should be a close paren
                tsParsedSexpr_t* body = name->next;
                if (!body || body->token != tsSexprPopList)
                    throw std::runtime_error("expected end of list");

                MachineExemplar* monad = NULL;
                compileMachine(body, l, mtMonad, &monad);
                if (!monad)
                    throw std::runtime_error("expected monad");

                MachineInstruction instr;
                instr.op = moSetVariable;
                instr.name = name->str;
                me->instructions.push_back(instr);
                instr.arg = NULL;
                instr.me = monad;
                return body->next;
            }
            break;
            case tsSexprAtom:
            case tsSexprString:
            case tsSexprFloat:
            case tsSexprInteger: {
                MachineInstruction instr;
                instr.op = moSetVariable;
                instr.name = name->str;
                me->instructions.push_back(instr);
                instr.me = NULL;
                instr.arg = body;
                return body->next;
            }
            break;
            case tsSexprPopList:
                throw std::runtime_error("unexpected end of list");
                break;
        }


        if (!body || body->token != tsSexprPopList)
            throw std::runtime_error("expected end of list");
        return body->next;
    }
    else {
        throw std::runtime_error("expected state name");
    }
}

void expectConstantOrList(tsParsedSexpr_t* curr) {
    if (!curr)
        throw std::runtime_error("expected a constant or list");
    switch (curr->token) {
        case tsSexprAtom:
        case tsSexprString:
        case tsSexprFloat:
        case tsSexprInteger:
        case tsSexprPushList:
            return;
        case tsSexprPopList:
            throw std::runtime_error("expected a constant or list");
    }
}

tsParsedSexpr_t* compileFunctionCall(tsParsedSexpr_t* curr, Landru* l, MachineExemplar* me) {
    // compile a function call. The existance or non-existance
    // of the function will be discovered at runtime.
    tsParsedSexpr_t* name = curr;
    curr = curr->next;
    expectConstantOrList(curr);

    MachineInstruction instr;
    instr.op = moFunctionCall;
    instr.name = name->str;
    me->instructions.push_back(instr);
    instr.me = NULL;
    instr.arg = NULL;
    MachineExemplar* monad = NULL;
    if (curr->token == tsSexprPushList) {
        curr = compileMachine(curr, l, mtList, &monad);
        if (!monad)
            diagnoseAndThrow(curr, "expected monad");
        instr.me = monad;
    }
    else {
        instr.arg = curr;
        curr = curr->next; // consume value
        if (!curr || curr->token != tsSexprPopList)
            diagnoseAndThrow(curr, "expected closing paren");
    }
    return curr;
}

tsParsedSexpr_t* compileRequire(tsParsedSexpr_t* curr, Landru* l) {
    // require statements are immediately executed because they have global
    // scope. they aren't compiled into the current machine.
    // peek at next to get the name of the required library
    tsParsedSexpr_t* name = curr->next;
    if (name && name->token == tsSexprAtom) {
        std::string nameStr = std::string(name->str.curr, name->str.sz);
        registerLibrary(l, nameStr.c_str());
        name = name->next; // consume the name
        if (!name || name->token != tsSexprPopList)
            diagnoseAndThrow(curr, "expected paren at end of scope");
        return name; // consume the closing paren
    }
    else
        diagnoseAndThrow(curr, "expected library name");
}

// parse given an open parens token. Return the corresponding close paren.
//
tsParsedSexpr_t* compileMachine(
    tsParsedSexpr_t* curr, 
    Landru* l, 
    MachineType mt, 
    MachineExemplar** exemplar) {
    
    if (!curr || !l || (curr->token != tsSexprPushList))
        diagnoseAndThrow(curr, "expected an open paren\n");

    curr = curr->next;
    if (!curr)
        diagnoseAndThrow(curr, "unexpected end of program\n");
    
    // the top level machine is implicit and unnamed.
    std::string name = "top";
    // create new substate
    MachineExemplar* me = new MachineExemplar();
    me->isState = mt;
    *exemplar = me;

    if (mt == mtList) {
    }
    else if (mt == mtRoot) {
    }
    else {
        // machine, state, defun, and defum expect to be followed by a name
        if (curr->token != tsSexprAtom)
            diagnoseAndThrow(curr, "expected machine atom");
        std::string token = std::string(curr->str.curr, curr->str.sz);
        switch (mt) {
            case mtMachine:
                if (token != "machine")
                    diagnoseAndThrow(curr, "expected machine atom");
                break;
            case mtState:
                if (token != "state")
                    diagnoseAndThrow(curr, "expected state atom");
                break;
            case mtFunction:
                if (token != "defun")
                    diagnoseAndThrow(curr, "expected defun atom");
                break;
            case mtMonad:
                if (token != "defum")
                    diagnoseAndThrow(curr, "expected defum atom");
                break;
            case mtOn:
                if (token != "on")
                    diagnoseAndThrow(curr, "expected defum atom");
                break;
            case mtRoot:
                diagnoseAndThrow(curr, "root must be at root position");
            case mtList:
                // anything is valid in the next position
                break;
        }
        curr = curr->next;
        if (!curr || curr->token != tsSexprAtom)
            diagnoseAndThrow(curr, "expected state name");
        name = std::string(curr->str.curr, curr->str.sz);
        curr = curr->next;
    }
    me->name = name;

    if (mt == mtOn) {
        // the name of the event is already in name.
        // The next token is a constant value.
        switch (curr->token) {
            case tsSexprAtom:
            case tsSexprString:
            case tsSexprFloat:
            case tsSexprInteger:
                break;
            default:
                diagnoseAndThrow(curr, "expected constant value to parameterize condition");
        }
        me->arg = curr;
        curr = curr->next;
        if (!curr || curr->token != tsSexprPushList)
            diagnoseAndThrow(curr, "expected body to follow on condition\n");
    }

    // at this level in the parsing, there are things that are compile time;
    // require statements are evaluated immediately, as are function calls
    // outside of states.
    //
    // other things are compiled for use at runtime. These include function
    // declarations, and nested machine definitions.
    //
    // states are compiled into the state table
    //
    // goto to statements are added to the local state as a pending goto
    // and are evaluated by the runtime the next time the runtime runs.
    //
    // launch statements are appended into the launch list, and are evaluated
    // by the runtime the next time the run time runs.

    int level = 0;
    tsParsedSexpr_t* openParen = NULL;
    while (curr && level >= 0) {
        switch (curr->token) {
            case tsSexprPushList:
                openParen = curr;
                curr = curr->next;
                level++;
                continue;

            case tsSexprPopList:
                if (!level)
                    return curr;
                else if (level == 1)
                    return curr->next;
                openParen = NULL;
                curr = curr->next;
                level--;
                continue;

            case tsSexprAtom: {
                std::string s = std::string(curr->str.curr, curr->str.sz);
                if (s == "machine") {
                    // parse a local machine
                    MachineExemplar* sub_me = NULL;
                    tsParsedSexpr_t* verify = findClosingParen(openParen);
                    curr = compileMachine(openParen, l, mtMachine, &sub_me);
                    if (verify != curr)
                        diagnoseAndThrow(curr, "machine not properly closed");
                    if (sub_me) {
                        // and make it available to the local scope
                        sub_me->parent = me;
                        sub_me->isState = mtMachine;
                        me->machines[sub_me->name] = sub_me;
                    }
                    curr = curr->next;
                    --level;
                }
                else if (s == "state") {
                    // parse a local state; the only difference between a state and
                    // a machine is that machines are launched into the same
                    // scope as the launching machine, and states are launched
                    // into the local scope of the machine.
                    // Additionally, switching into a state terminates the
                    // present state, and launching a machine does not affect the
                    // machine doing the launching.
                    MachineExemplar* sub_me = NULL;
                    tsParsedSexpr_t* verify = findClosingParen(openParen);
                    curr = compileMachine(openParen, l, mtState, &sub_me);
                    if (verify != curr)
                        diagnoseAndThrow(curr, "state not properly closed");
                    if (sub_me) {
                        // and make it available to the local scope
                        sub_me->parent = me;
                        sub_me->isState = mtState;
                        me->machines[sub_me->name] = sub_me;
                    }
                    curr = curr->next;
                    --level;
                }
                else if (s == "on") {
                    MachineExemplar* sub_me = NULL;
                    tsParsedSexpr_t* verify = findClosingParen(openParen);
                    curr = compileMachine(openParen, l, mtOn, &sub_me);
                    if (verify != curr)
                        diagnoseAndThrow(curr, "on not properly closed");
                    if (sub_me) {
                        // and make it available to the local scope
                        sub_me->parent = me;
                        sub_me->isState = mtState;
                        me->ons.emplace_back(sub_me);
                    }
                    curr = curr->next;
                    --level;
                }
                else if (s == "require") {
                    tsParsedSexpr_t* verify = findClosingParen(openParen);
                    curr = compileRequire(curr, l);
                    if (verify != curr)
                        diagnoseAndThrow(curr, "require not properly closed");
                    curr = curr->next;
                    --level;
                }
                else if (s == "defun") {
                    // a function is just like a state, except invoking it
                    // does not terminate the present state or launch a new one.
                    // a function is a pure function, and does not have access to
                    // the local scope of the machine that invoked it.
                    MachineExemplar* sub_me = NULL;
                    tsParsedSexpr_t* verify = findClosingParen(openParen);
                    curr = compileMachine(openParen, l, mtFunction, &sub_me);
                    if (verify != curr)
                        diagnoseAndThrow(curr, "defun not properly closed");
                    if (sub_me) {
                        // and make it available to the local scope
                        sub_me->parent = me;
                        sub_me->isState = mtFunction;
                        me->functions[sub_me->name] = sub_me;
                    }
                    curr = curr->next;
                    --level;
                }
                else if (s == "defum") {
                    // a monadic function is just like a function, except that
                    // it may have side effects.
                    MachineExemplar* sub_me = NULL;
                    tsParsedSexpr_t* verify = findClosingParen(openParen);
                    curr = compileMachine(openParen, l, mtMonad, &sub_me);
                    if (verify != curr)
                        diagnoseAndThrow(curr, "defum not properly closed");
                    if (sub_me) {
                        // and make it available to the local scope
                        sub_me->parent = me;
                        sub_me->isState = mtMonad;
                        me->functions[sub_me->name] = sub_me;
                    }
                    curr = curr->next;
                    --level;
                }
                else if (s == "launch") {
                    // compile a launch operation
                    // note that at the moment a launch occurs into the local scope
                    // and not a higher scope. The next iteration of the compiler
                    // will include sytnax to allow a launch to occur into a specific
                    // scope. Perhaps there will be named scopes, or perhaps a scheme
                    // whereby a sequence of carets might indicate the number of 
                    // parent scopes up to launch into. Maybe both. Needs a bit more
                    // thought. Perhaps an operation might be defineScope, which
                    // would name a scope. An example use case might be
                    // (scope playfield)
                    // and a player machine might launch bubble machines into the
                    // playfield scope. That's probably the best idea.
                    tsParsedSexpr_t* verify = findClosingParen(openParen);
                    curr = compileLaunch(curr, me);
                    if (verify != curr)
                        diagnoseAndThrow(curr, "launch not properly closed");
                    curr = curr->next;
                    --level;
                }
                else if (s == "goto") {
                    tsParsedSexpr_t* verify = findClosingParen(openParen);
                    curr = compileGoto(curr, me);
                    if (verify != curr)
                        diagnoseAndThrow(curr, "goto not properly closed");
                    curr = curr->next;
                    --level;
                }
                else if (s == "set") {
                    tsParsedSexpr_t* verify = findClosingParen(openParen);
                    curr = compileSet(curr, l, me);
                    if (verify != curr)
                        diagnoseAndThrow(curr, "set not properly closed");
                    curr = curr->next;
                    --level;
                }
                else {
                    tsParsedSexpr_t* verify = findClosingParen(openParen);
                    curr = compileFunctionCall(curr, l, me);
                    if (verify != curr)
                        diagnoseAndThrow(curr, "function is not properly closed");
                    curr = curr->next;
                    --level;
                }
                break;
            }

            case tsSexprString:
            case tsSexprFloat:
            case tsSexprInteger:
                break;
        }
    }

    *exemplar = me;
    return curr;
}


bool LandruRun(const char* src, bool dump_exemplar) {
    tsParsedSexpr_t* parsed = tsParsedSexpr_New();
    parsed->token = tsSexprAtom;
    tsStrView_t str = (tsStrView_t){src, strlen(src)};

    /*tsStrView_t end_of_str =*/ tsStrViewParseSexpr(&str, parsed, 0);
    dump_parsed(parsed);

    Landru l;
    l.root.me = NULL;
    l.root.localStates.push_back({});
    MachineExemplar* me = NULL;

    // @TODO get rid of the empty token in the parser itself
    if (parsed->token == tsSexprAtom && parsed->str.sz == 0)
        parsed = parsed->next;

    // empty file is valid
    if (!parsed)
        return true;
    
    tsParsedSexpr_t open = {tsSexprPushList, };
    tsParsedSexpr_t close = {tsSexprPopList, };

    open.next = parsed;
    tsParsedSexpr_t* last = parsed;
    while (last) {
        if (!last->next) {
            last->next = &close;
            break;
        }
        last = last->next;
    }

    tsParsedSexpr_t* verifyClose = findClosingParen(&open);
    if (verifyClose != &close) {
        diagnoseAndThrow(verifyClose, "unmatched open paren");
    }

    tsParsedSexpr_t* curr = compileMachine(&open, &l, mtRoot, &me);
    if (curr != verifyClose) {
        diagnoseAndThrow(curr, "found tokens beyond end of script");
    }

    if (dump_exemplar) {
        dumpExemplar(me);
    }
    return false;
}

bool LandruRun(const char* src) {
    return LandruRun(src, false);
}

bool LandruRunPrintExemplar(const char* src) {
    return LandruRun(src, true);
}


int landru_tests() {
    printf("test empty program\n");
    LandruRun("");
    printf("test empty list\n");
    LandruRun("()");
    printf("test print\n");
    LandruRun("(io.print \"(\")");
    LandruRun("(io.print \")\")");
    LandruRun("(io.print \"()\")");
    LandruRun("(io.print \"(\") (io.print \")\")");
    printf("test require parsing\n");
    LandruRun("(require time) (require io)");
    LandruRun("(goto foo) (goto bar)");
    printf("test machine\n");
    LandruRun("(require test) (machine main (state ping ()) (state pong())) (launch main)");
    printf("test ping pong\n");
    LandruRun(pingpongSrc, true);
    return 0;
}
