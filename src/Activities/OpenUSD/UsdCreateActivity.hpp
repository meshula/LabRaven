//
//  CreateActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 12/1/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#ifndef CreateActivity_hpp
#define CreateActivity_hpp

#include "Lab/StudioCore.hpp"
#include <pxr/usd/sdf/path.h>

namespace lab {
class UsdCreateActivity : public Activity
{
    struct data;
    data* _self;

    // activities
    void Menu();
    void RunUI(const LabViewInteraction&);
    
public:
    UsdCreateActivity();
    virtual ~UsdCreateActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "UsdCreate"; }
};

}
#endif /* CreateActivity_hpp */
