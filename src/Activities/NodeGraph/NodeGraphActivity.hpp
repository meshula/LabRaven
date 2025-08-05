//
//  NodeGraphActivity.hpp
//  LabRaven
//
//  Created by Nick Porcino on 8/5/25.
//  Copyright © 2025 Nick Porcino. All rights reserved.
//

#ifndef ACTIVITIES_NODEGRAPHACTIVITY_HPP
#define ACTIVITIES_NODEGRAPHACTIVITY_HPP

#include "Lab/StudioCore.hpp"

namespace lab {

class NodeGraphActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);
    virtual void _activate() override;
    virtual void _deactivate() override;

public:
    explicit NodeGraphActivity();
    virtual ~NodeGraphActivity();
    
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "NodeGraph"; }
};

} // namespace lab

#endif /* ACTIVITIES_NODEGRAPHACTIVITY_HPP */
