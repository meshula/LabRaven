//
//  StatisticsActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 2/11/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef StatisticsActivity_hpp
#define StatisticsActivity_hpp

#include "Lab/StudioCore.hpp"

namespace lab {

class StatisticsActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);

public:
    explicit StatisticsActivity();
    virtual ~StatisticsActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Usd Statistics"; }
};

} // lab

#endif /* StatisticsActivity_hpp */
