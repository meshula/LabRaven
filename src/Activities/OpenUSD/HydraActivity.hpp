//
//  HydraActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 1/7/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef HydraActivity_hpp
#define HydraActivity_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
class HydraActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);
    void Menu();

public:
    HydraActivity();
    virtual ~HydraActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Hydra"; }
};
}

#endif /* HydraActivity_hpp */
