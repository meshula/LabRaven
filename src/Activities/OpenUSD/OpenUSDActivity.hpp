//
//  OpenUSDActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 1/7/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef OpenUSDActivity_hpp
#define OpenUSDActivity_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
class OpenUSDActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);
    void Menu();

public:
    OpenUSDActivity();
    virtual ~OpenUSDActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "OpenUSD"; }
};
}

#endif /* OpenUSDActivity_hpp */
