//
//  PrimPropertiesActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 1/7/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef PrimPropertiesActivity_hpp
#define PrimPropertiesActivity_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
class PrimPropertiesActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);

public:
    PrimPropertiesActivity();
    virtual ~PrimPropertiesActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Prim Properties"; }
};
}

#endif /* PropertiesActivity_hpp */
