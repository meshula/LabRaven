
#include "PreferencesActivity.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

#include "Lab/LabDirectories.h"

namespace lab {

struct PreferencesActivity::data
{
};

PreferencesActivity::PreferencesActivity() : Activity(PreferencesActivity::sname())
{
    _self = new data;

    // Set the RunUI function
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<PreferencesActivity*>(instance)->RunUI(*vi);
    };
}

PreferencesActivity::~PreferencesActivity()
{
    delete _self;
}

void PreferencesActivity::RunUI(const LabViewInteraction& interaction)
{
    ImGui::Begin("Preferences##W");

    LabPreferences prefs = LabPreferencesLock();

    std::string prefs_dir = lab_application_preferences_path(nullptr);
    if (ImGui::Button("[>]")) {
        lab_reveal_on_desktop(prefs_dir.c_str());
    }
    ImGui::SameLine();
    ImGui::Text("Prefs dir: %s", prefs_dir.c_str());
    ImGui::Separator();

    // if there are no preferences, display a message
    if (prefs.prefs.empty()) {
        ImGui::Text("No preferences yet.");
    }
    else {
        // Display the preferences
        for (const auto& pref : prefs.prefs) {
            ImGui::Text("%s: %s", pref.first.c_str(), pref.second.c_str());
        }
    }
    ImGui::End();
}

} // lab
