//
//  JournalActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 1/7/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef JournalActivity_hpp
#define JournalActivity_hpp

#include "Lab/StudioCore.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

namespace lab {
class JournalActivity : public Activity
{
    struct data;
    data* _self;
    ImRect DrawJournalHierarchy(JournalNode*);
    bool DrawJournalNode(JournalNode*);
    void DrawChildrendHierarchyDecoration(ImRect parentRect,
                                          std::vector<ImRect> childrenRects);
    ImGuiTreeNodeFlags ComputeDisplayFlags(JournalNode*);
    
    // activities
    void RunUI(const LabViewInteraction&);

public:
    JournalActivity();
    virtual ~JournalActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr std::string sname() { return "Journal"; }
};
}

#endif /* JournalActivity_hpp */
