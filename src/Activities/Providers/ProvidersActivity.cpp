
#include "ProvidersActivity.hpp"
#include "Lab/StudioCore.hpp"
#include "Lab/App.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

namespace lab {

struct ProvidersData {
};

struct ProvidersActivity::data
{
    std::vector<Provider*> providers;
    std::vector<std::string> providerNames;
    int selectedProvider = -1;
};

ProvidersActivity::ProvidersActivity() : Activity(ProvidersActivity::sname()) {
    _self = new data;

    auto orchestrator = Orchestrator::Canonical();
    auto providerNames = orchestrator->ProviderNames();
    for (auto& name : providerNames) {
        auto provider = orchestrator->FindProvider(name);
        if (provider) {
            _self->providers.push_back(provider);
            _self->providerNames.push_back(provider->Name() + "##PLB");
            printf("%s\n", _self->providerNames.back().c_str());
        }
    }

    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<ProvidersActivity*>(instance)->RunUI(*vi);
    };
}

ProvidersActivity::~ProvidersActivity() {
    delete _self;
}

void ProvidersActivity::RunUI(const LabViewInteraction& interaction) {
    ImGui::Begin("Providers##W");
    ImGui::BeginListBox("Providers##LB", {200, -1});
    // make a selectable list of providers. When one is selected, display its documentation in the second group.
    int item = 0;
    for (auto& provider : _self->providerNames) {
        const bool is_selected = (_self->selectedProvider == item);
        if (ImGui::Selectable(provider.c_str(), is_selected)) {
            _self->selectedProvider = item;
        }
        if (is_selected)
            ImGui::SetItemDefaultFocus();
        item++;
    }
    ImGui::EndListBox();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextUnformatted("Documentation");
    if (_self->selectedProvider >= 0 && _self->selectedProvider < _self->providers.size()) {
        auto provider = _self->providers[_self->selectedProvider];
        if (provider->provider.Documentation) {
            ImGui::TextUnformatted(provider->provider.Documentation((void*) provider));
        }
    }
    ImGui::EndGroup();
    ImGui::End();
}

} // lab
