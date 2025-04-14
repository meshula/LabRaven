//
//  HydraPropertiesActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 1/7/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef PropertiesActivity_hpp
#define PropertiesActivity_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
class HydraPropertiesActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);

public:
    HydraPropertiesActivity();
    virtual ~HydraPropertiesActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Hydra Properties"; }
};

} // lab

#endif /* PropertiesActivity_hpp */
