
#include "AudioContextManager.hpp"
#include "SoundActivity.hpp"
#include "Lab/App.h"
#include "Providers/Sound/SoundProvider.hpp"
#include "Lab/LabDirectories.h"
#include "Lab/ImguiExt.hpp"

#include "LabSound/LabSound.h"
#include "LabSound/extended/Util.h"

#include <map>
#include <string>
#include <vector>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

const int kMaxDevices = 32;

using namespace lab;

struct AudioContextManager::data {
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
    std::vector<int> input_reindex;
    std::vector<int> output_reindex;

    int input  = -1;
    int output = -1;
    bool input_checks[kMaxDevices];
    bool output_checks[kMaxDevices];

    std::vector<AudioDeviceInfo> info;

    data() {
        info = lab::SoundProvider::instance()->DeviceInfo();
        int reindex = 0;
        int unique = 0;
        input = -1;
        output = -1;
        for (auto& i : info) {
            if (i.num_input_channels > 0) {
                inputs.push_back(i.identifier + "##" + std::to_string(unique++));
                input_reindex.push_back(reindex);
            }
            if (i.num_output_channels > 0) {
                outputs.push_back(i.identifier + "##" + std::to_string(unique++));
                output_reindex.push_back(reindex);
            }
            ++reindex;
        }

        for (int i = 0; i < kMaxDevices; ++i) {
            input_checks[i] = false;
            output_checks[i] = false;
        }

        int deviceCount = (int) inputs.size();
        if (deviceCount > kMaxDevices) {
            deviceCount = kMaxDevices;
        }
        for (int i = 0; i < deviceCount; ++i) {
            const auto& device = info[i];
            if (device.num_input_channels > 0 && device.is_default_input) {
                input_checks[i] = true;
            }
            if (device.num_output_channels > 0 && device.is_default_output) {
                output_checks[i] = true;
            }
        }
    }

    void RunUI() {
        auto ac = lab::SoundProvider::instance()->Context();

        static float leftWidth = 300;
        static float rightWidth = 300;
        ImGui::BeginChild("###ACM", ImVec2{leftWidth + rightWidth, 80});
        ImGui::BeginChild("##ACM-Inputs", ImVec2{ leftWidth, 0 });
        ImGui::TextUnformatted("Inputs");
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
        for (int j = 0; j < inputs.size(); ++j) {
            bool set = input_checks[j];
            if (ImGui::Checkbox(inputs[j].c_str(), &input_checks[j])) {
                if (!ac) {
                    for (int i = 0; i < inputs.size(); ++i)
                        input_checks[i] = i == j;
                }
                else {
                    input_checks[j] = set;
                }
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();

        //imgui_splitter(true, 8, &leftWidth, &rightWidth, 80, 80);

        ImGui::BeginChild("##ACM-Outputs", ImVec2{ rightWidth, 0 });
        ImGui::TextUnformatted("Outputs");
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
        for (int j = 0; j < outputs.size(); ++j) {
            bool set = output_checks[j];
            if (ImGui::Checkbox(outputs[j].c_str(), &output_checks[j])) {
                if (!ac) {
                    for (int i = 0; i < outputs.size(); ++i)
                        output_checks[i] = i == j;
                }
                else {
                    input_checks[j] = set;
                }
            }
        }
        ImGui::EndChild();
        ImGui::EndChild();

        if (ac) {
            ImGui::TextUnformatted("Context:");
            ImGui::Text("  Input:  %s", input  <0? "none" : info[input].identifier.c_str());
            ImGui::Text("  Output: %s", output <0? "none" : info[output].identifier.c_str());
        }
        else {
            if (ImGui::Button("Create Context")) {
                AudioStreamConfig inputConfig;
                for (int i = 0; i < inputs.size(); ++i) {
                    if (input_checks[i]) {
                        int r = input_reindex[i];
                        inputConfig.device_index = r;
                        inputConfig.desired_channels = info[r].num_input_channels;
                        inputConfig.desired_samplerate = info[r].nominal_samplerate;
                        input = i;
                        break;
                    }
                }
                AudioStreamConfig outputConfig;
                for (int i = 0; i < outputs.size(); ++i) {
                    if (output_checks[i]) {
                        int r = output_reindex[i];
                        outputConfig.device_index = r;
                        outputConfig.desired_channels = info[r].num_output_channels;
                        outputConfig.desired_samplerate = info[r].nominal_samplerate;
                        output = i;
                        break;
                    }
                }
                if (outputConfig.device_index >= 0) {
                    SoundProvider::instance()->CreateContext(inputConfig, outputConfig);
                }
            }
        }
    }

};

AudioContextManager::AudioContextManager() {
    _self = new data();
}

AudioContextManager::~AudioContextManager() {
    delete _self;
}

void AudioContextManager::RunUI()
{
    _self->RunUI();
}
