//
//  LayersActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 2/11/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef LayersActivity_hpp
#define LayersActivity_hpp

#include "Lab/StudioCore.hpp"

namespace lab {

class LayersActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);
    void Menu();

public:
    explicit LayersActivity();
    virtual ~LayersActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Layers"; }
};

} // lab

#endif /* LayersActivity_hpp */
