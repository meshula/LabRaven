
#include "PluginActivity.hpp"
#include "Lab/StudioCore.hpp"
#include "Lab/LabDirectories.h"
#include "Lab/App.h"

#include "imgui.h"

#define CR_NO_ROLLBACK
#define CR_HOST
#include "cr.h"

#include <sys/stat.h>
#include <filesystem>
#include <vector>

namespace lab {

class PluginActivityInstance : public Activity
{
public:
    explicit PluginActivityInstance(const LabActivity& activity_)
        : Activity(activity_.name) {
            activity = activity_;
        }

    virtual const std::string Name() const override {
        return activity.name;
    }
};

struct PluginData {
    std::string name;
    std::string plugin_filename;
    bool enabled = false;
    bool loaded = false;
    cr_plugin ctx;
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

PluginActivity::PluginActivity() : Activity(PluginActivity::sname()) {
    _self = new data;

    // look in the plugins directory for any plugins, recognizable by a .dll or .so extension.
    std::string dir = _self->plugins_dir();

    std::vector<std::filesystem::path> found_plugins;
    struct stat info;
    if (stat(dir.c_str(), &info) == 0 && S_ISDIR(info.st_mode)) {
        // set _self->temp_found_plugins_dir to true if the plugins directory was found
        _self->found_plugins_dir = true;
        // use std::filesystem to gather any plugins in the plugins directory
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".dll" || ext == ".so" || ext == ".dylib") {
                    found_plugins.push_back(entry.path());
                }
            }
        }
    }
    else {
        _self->found_plugins_dir = false;
    }

    // load each plugin found in the plugins directory
    for (const auto& plugin : found_plugins) {
        PluginData pd;
        pd.name = plugin.filename().string();
        pd.plugin_filename = plugin.filename().string();

        // load the plugin
        pd.loaded = cr_plugin_open(pd.ctx, plugin.string().c_str());
        pd.enabled = pd.loaded;

        // the plugin may have a function const char* PluginName() that returns 
        // the name of the plugin, fetch it using cr_so_symbol.
        if (pd.loaded) {
            if (0 == cr_plugin_update(pd.ctx)) {   // causes the plugin to load.
                auto plugInNameFn = cr_so_symbol<const char*(*)(void)>(pd.ctx, "PluginName");
                if (plugInNameFn) {
                    pd.name = plugInNameFn();
                }

                // the plugin may contain activities. If it does, the function
                // LabActivity make_activity() should be called to create the activity.
                auto makeActivityFn = cr_so_symbol<LabActivity(*)(void)>(pd.ctx, "make_activity");
                if (makeActivityFn) {
                    LabActivity activity = makeActivityFn();
                    Activity* act = new PluginActivityInstance(activity);
                    // register it with the orchestrator by name.
                    auto orchestrator = Orchestrator::Canonical();
                    orchestrator->RegisterActivity(activity.name, [act]() {
                        return std::shared_ptr<Activity>(act);
                    });
                }
            }
            else {
                pd.enabled = false;
            }
        }

        _self->plugins.push_back(pd);
    }

    // add some dummy plugins for testing the UI
    _self->plugins.push_back({"Plugin 1", "built in", true});
    _self->plugins.push_back({"Plugin 2", "built in", false});
    _self->plugins.push_back({"Plugin 3", "built in", true});

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

    if (ImGui::Button("[>]")) {
        lab_reveal_on_desktop(dir.c_str());
    }
    ImGui::SameLine();
    ImGui::Text("Plugin dir: %s", dir.c_str());

    if (!_self->found_plugins_dir) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Plugin directory not found!");
    }

    ImGui::Separator();

    // Display the list of plugins, with a checkbox to enable/disable each one
    for (auto& plugin : _self->plugins) {
        // if the plugin was not loaded, display it in red
        if (!plugin.loaded) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", plugin.name.c_str());
            ImGui::SameLine();
            ImGui::TextUnformatted(" (not loaded)");
        }
        else {
            ImGui::Checkbox(plugin.name.c_str(), &plugin.enabled);
            ImGui::SameLine();
            ImGui::Text(" (%s)", plugin.plugin_filename.c_str());
        }
    }

    ImGui::End();
}

} // lab
