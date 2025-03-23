
#include "SoundActivity.hpp"
#include "PlaySoundFile.hpp"
#include "AudioContextManager.hpp"

#include "Lab/App.h"
#include "Providers/Sound/SoundProvider.hpp"
#include "Lab/LabDirectories.h"

#include "LabSound/LabSound.h"
#include "LabSound/extended/Util.h"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <iostream>
#include <string>

namespace lab {


struct SoundActivity::data {
    data() {
        acm = new AudioContextManager();
    }
    ~data() {
        if (playSoundFile) { delete playSoundFile; }
        if (acm) { delete acm; }
    }
    std::map<std::string, std::shared_ptr<AudioBus>> sample_cache;
    PlaySoundFile* playSoundFile = nullptr;
    AudioContextManager* acm = nullptr;
};

SoundActivity::SoundActivity() 
: Activity(SoundActivity::sname())
, _self(new data) {
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<SoundActivity*>(instance)->RunUI(*vi);
    };
}

SoundActivity::~SoundActivity() {
    delete _self;
}

void SoundActivity::_activate() {
}

void SoundActivity::_deactivate() {
}

void SoundActivity::RunUI(const LabViewInteraction&) {
    if (!_self->playSoundFile) {
        _self->playSoundFile = new PlaySoundFile();
    }

    ImGui::Begin("LabSound");
    _self->acm->RunUI();
    ImGui::End();
}

} // namespace lab
