//
//  SessionActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 1/7/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef SessionActivity_hpp
#define SessionActivity_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
class SessionActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);

public:
    SessionActivity();
    virtual ~SessionActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "UsdSession"; }
};
}

#endif /* SessionActivity_hpp */
