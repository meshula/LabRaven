//
//  ScoutActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 2/11/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef ScoutActivity_hpp
#define ScoutActivity_hpp

#include "Lab/StudioCore.hpp"

namespace lab {

class ScoutActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);
    void Menu();

public:
    explicit ScoutActivity();
    virtual ~ScoutActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Scout"; }
};

} // lab

#endif /* ScoutActivity_hpp */
