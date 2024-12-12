//
//  CoreNodeTestActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 12/6/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#ifndef CoreNodeTestActivity_hpp
#define CoreNodeTestActivity_hpp

#include "Lab/StudioCore.hpp"

namespace lab {

class CoreNodeTestActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);

public:
    explicit CoreNodeTestActivity();
    virtual ~CoreNodeTestActivity() = default;
    
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "CoreNodeTestActivity"; }
};

} // lab

#endif /* CoreNodeTestActivity */
