
#include "PluginActivity.hpp"
#include "Lab/StudioCore.hpp"
#include "Lab/LabDirectories.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

#include <sys/stat.h>
#include <vector>

namespace lab {

struct PluginData {
    std::string name;
    bool enabled;
};

struct PluginActivity::data
{
    std::vector<PluginData> plugins;
    bool found_plugins_dir;

    std::string plugins_dir() const {
        std::string dir = lab_application_executable_path(nullptr);
        dir = dir.substr(0, dir.find_last_of("/\\"));
        dir += "/plugins";
        return dir;
    }
};

PluginActivity::PluginActivity() {
    _self = new data;

    _self->plugins.push_back({"Plugin 1", true});
    _self->plugins.push_back({"Plugin 2", false});
    _self->plugins.push_back({"Plugin 3", true});

    // set _self->temp_found_plugins_dir to true if the plugins directory was found
    // test for the existance of the plugins directory
    std::string dir = _self->plugins_dir();
    struct stat info;
    if (stat(dir.c_str(), &info) == 0 && S_ISDIR(info.st_mode)) {
        _self->found_plugins_dir = true;
    } else {
        _self->found_plugins_dir = false;
    }

    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<PluginActivity*>(instance)->RunUI(*vi);
    };
}

PluginActivity::~PluginActivity() {
    delete _self;
}

void PluginActivity::RunUI(const LabViewInteraction& interaction) {
    ImGui::Begin("Plugins##W");

    // Display the plugin directory, which is the application direction + "/plugins"
    std::string dir = lab_application_executable_path(nullptr);
    dir = dir.substr(0, dir.find_last_of("/\\"));
    dir += "/plugins";

    ImGui::Text("Plugin Directory: %s", dir.c_str());

    if (!_self->found_plugins_dir) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Plugins directory not found!");
    }

    // Display the list of plugins, with a checkbox to enable/disable each one
    for (auto& plugin : _self->plugins) {
        ImGui::Checkbox(plugin.name.c_str(), &plugin.enabled);
    }

    ImGui::End();
}

} // lab
