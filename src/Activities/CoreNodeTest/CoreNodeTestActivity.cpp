#include "CoreNodeTestActivity.hpp"

#include "CoreDiagram.hpp"

namespace lab {

struct CoreNodeTestActivity::data {
    data() {
        coreDiagram = std::make_unique<CoreDiagram>();
    }
    std::unique_ptr<CoreDiagram> coreDiagram;
};

CoreNodeTestActivity::CoreNodeTestActivity()
: Activity(CoreNodeTestActivity::sname()) {
    _self = new CoreNodeTestActivity::data;

    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<CoreNodeTestActivity*>(instance)->RunUI(*vi);
    };
}

void CoreNodeTestActivity::RunUI(const LabViewInteraction&)
{
    ImGui::Begin("Library", nullptr, ImGuiWindowFlags_None);
    _self->coreDiagram->DrawLibrary();
    ImGui::End();

    ImGui::Begin("Explorer", nullptr, ImGuiWindowFlags_None);
    _self->coreDiagram->DrawExplorer();
    ImGui::End();

    ImGui::Begin("Diagram", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    _self->coreDiagram->Update();
    ImGui::End();

    ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_None);
    _self->coreDiagram->DrawProperties();
    ImGui::End();
}

} // lab
