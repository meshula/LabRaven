//
//  DebuggerActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 24/02/09
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef DebuggerActivity_hpp
#define DebuggerActivity_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
    class DebuggerActivity : public Activity
    {
        struct data;
        data* _self;
        
        // activites
        void RunUI(const LabViewInteraction&);
        void Menu();

    public:
        explicit DebuggerActivity();
        virtual ~DebuggerActivity();
        virtual const std::string Name() const override { return sname(); }
        static constexpr const char* sname() { return "TfDebugger"; }
    };
}

#endif /* DebuggerActivity_hpp */