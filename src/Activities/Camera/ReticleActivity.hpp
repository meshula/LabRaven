//
//  ReticleActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 11/29/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#ifndef ReticleActivity_hpp
#define ReticleActivity_hpp

#include "Lab/StudioCore.hpp"
#include "imgui.h"

namespace lab {
class ReticleActivity : public Activity
{
    struct data;
    data* _self;

    // activities
    void ToolBar();
    void RunUI(const LabViewInteraction&);

public:
    ReticleActivity();
    virtual ~ReticleActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Reticle"; }

    void Render(ImDrawList* draw_list, float w, float h);
};
}

#endif /* ReticleActivity_hpp */
