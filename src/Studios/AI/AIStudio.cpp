//
//  Studios/AI/AIStudio.cpp
//  LabRaven
//
//  AI Studio - Activates AI and LLM-related activities
//

#include "AIStudio.hpp"
#include <iostream>

namespace lab {
struct AIStudio::data {
};

AIStudio::AIStudio() : Studio() {
    _self = new AIStudio::data();
}

AIStudio::~AIStudio() {
    delete _self;
}

static const std::vector<Studio::ActivityConfig> activities = {
    {"LlamaCpp", true}
};

const std::vector<Studio::ActivityConfig>& AIStudio::StudioConfiguration() const {
    return activities;
}

} // lab
