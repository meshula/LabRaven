//
//  ConsoleActivity.hpp
//  LabExcelsior
//
//  Created by Nick Porcino on 12/22/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#ifndef ConsoleActivity_hpp
#define ConsoleActivity_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
class ConsoleActivity : public Activity
{
    struct data;
    data* _self;
    
    // activites
    void RunUI(const LabViewInteraction&);
    void ToolBar();
    void Menu();

public:
    explicit ConsoleActivity();
    virtual ~ConsoleActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Console"; }

    //--------------------------------------------
    void Log(std::string_view);
    void Warning(std::string_view);
    void Error(std::string_view);
    void Info(std::string_view);
};
} // lab

#endif /* ConsoleActivity_hpp */
