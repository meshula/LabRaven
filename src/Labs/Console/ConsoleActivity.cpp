//
//  ConsoleActivity.cpp
//  LabExcelsior
//
//  Created by Nick Porcino on 12/22/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#include "ConsoleActivity.hpp"
#include "imgui_console/imgui_console.h"
#include "Lab/Landru.hpp"
#include "Lab/LabDirectories.h"
#include <mutex>

csys::ItemLog& operator << (csys::ItemLog &log, ImVec4& vec)
{
    log << "ImVec4: [" << vec.x << ", "
                       << vec.y << ", "
                       << vec.z << ", "
                       << vec.w << "]";
    return log;
}

static void imvec4_setter(ImVec4 & my_type, std::vector<int> vec)
{
    if (vec.size() < 4) return;

    my_type.x = vec[0]/255.f;
    my_type.y = vec[1]/255.f;
    my_type.z = vec[2]/255.f;
    my_type.w = vec[3]/255.f;
}

namespace lab {

struct ConsoleActivity::data {
    data() : console("Console") {}
    ~data() {}

    void init() {
        // Register variables
        console.System().RegisterVariable("background_color", clear_color, imvec4_setter);

        // Register scripts
        try {
            //console.System().RegisterScript("test_script", "./console.script");
        }
        catch(...) {
            std::cerr << "console.script not loaded\n";
        }
        
        // Register custom commands
        console.System().RegisterCommand(
                "random_background_color", 
                "Assigns a random color to the background application",
                [&clear_color = this->clear_color]()
                {
                    clear_color.x = (rand() % 256) / 256.f;
                    clear_color.y = (rand() % 256) / 256.f;
                    clear_color.z = (rand() % 256) / 256.f;
                    clear_color.w = (rand() % 256) / 256.f;
                });
        console.System().RegisterCommand(
                "reset_background_color", 
                "Reset background color to its original value",
                [&clear_color = this->clear_color, val = clear_color]()
                {
                    clear_color = val;
                });
        console.System().RegisterCommand(
                "dirs",
                "List directories",
                [this]()
                {
                    console.System().Log(csys::ItemType::INFO)
                        << "Listing directories" << csys::endl;
                    console.System().Log(csys::ItemType::INFO) << "exectuable: " <<
                        << lab_application_executable_path(nullptr) << csys::endl;
                    console.System().Log(csys::ItemType::INFO) << "resources: " <<
                        << lab_application_resource_path(nullptr, nullptr) << csys::endl;
                    console.System().Log(csys::ItemType::INFO) << "preferences: " <<
                        << lab_application_preferences_path(nullptr) << csys::endl;
                });

        // Log example information:
        console.System().Log(csys::ItemType::INFO) << "The following variables have been exposed to the console:" << csys::endl << csys::endl;
        console.System().Log(csys::ItemType::INFO) << "\tbackground_color - set: [int int int int]" << csys::endl;
        console.System().Log(csys::ItemType::INFO) << csys::endl << "Try running the following command:" << csys::endl;
        console.System().Log(csys::ItemType::INFO) << "\tset background_color [255 0 0 255]" << csys::endl << csys::endl;
        
        console.System().RegisterCommand("l", "Run landru program",
                                         [this](const csys::String &landru)
        {
            try {
                LandruRun(landru.m_String.c_str());
            }
            catch(std::exception& e) {
                console.System().Log(csys::ItemType::ERROR) << "Couldn't run the landru program" << csys::endl;

                console.System().Log(csys::ItemType::ERROR) << e.what() << csys::endl;
            }
        }, csys::Arg<csys::String>("landru"));
    }

    ImGuiConsole console;
    ImVec4 clear_color = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    std::once_flag must_init;
    bool ui_visible = true;
};

ConsoleActivity::ConsoleActivity()
: Activity()
, _self(new data)
{
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<ConsoleActivity*>(instance)->RunUI(*vi);
    };
    activity.ToolBar = [](void* instance) {
        static_cast<ConsoleActivity*>(instance)->ToolBar();
    };
    activity.Menu = [](void* instance) {
        static_cast<ConsoleActivity*>(instance)->Menu();
    };
}

ConsoleActivity::~ConsoleActivity()
{
    delete _self;
}

void ConsoleActivity::RunUI(const LabViewInteraction&)
{
    if (!_self->ui_visible)
        return;
    
    std::call_once(_self->must_init, [this]() {
        _self->init();
    });
    _self->console.Draw();
}

void ConsoleActivity::ToolBar() {
}

void ConsoleActivity::Menu() {
    if (ImGui::BeginMenu("Modes")) {
        if (ImGui::MenuItem("Console", nullptr, _self->ui_visible, true)) {
            _self->ui_visible = !_self->ui_visible;
        }
        ImGui::EndMenu();
    }
}

void ConsoleActivity::Log(std::string_view s) {
    _self->console.System().Log(csys::ItemType::LOG) << s << csys::endl;
}
void ConsoleActivity::Warning(std::string_view s) {
    _self->console.System().Log(csys::ItemType::WARNING) << s << csys::endl;
}
void ConsoleActivity::Error(std::string_view s) {
    _self->console.System().Log(csys::ItemType::ERROR) << s << csys::endl;
}
void ConsoleActivity::Info(std::string_view s) {
    _self->console.System().Log(csys::ItemType::INFO) << s << csys::endl;
}


} // lab
