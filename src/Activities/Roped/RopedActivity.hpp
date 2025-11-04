//
//  RopedActivity.hpp
//  LabRaven
//
//  Created by Nick Porcino on 8/5/25.
//  Copyright © 2025 Nick Porcino. All rights reserved.
//

#ifndef ACTIVITIES_ROPEDACTIVITY_HPP
#define ACTIVITIES_ROPEDACTIVITY_HPP

#include "Lab/StudioCore.hpp"

namespace lab {

class RopedActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);
    virtual void _activate() override;
    virtual void _deactivate() override;

public:
    explicit RopedActivity();
    virtual ~RopedActivity();
    
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Roped"; }
};

} // namespace lab

#endif /* ACTIVITIES_ROPEDACTIVITY_HPP */
