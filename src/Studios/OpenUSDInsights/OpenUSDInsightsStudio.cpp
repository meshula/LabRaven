//
//  Studios/TextureStudio.cpp
//  LabExcelsior
//
//  Created by Nick Porcino on 3/2/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#include "OpenUSDInsightsStudio.hpp"
#include <iostream>

namespace lab {
struct OpenUSDInsightsStudio::data {
};

OpenUSDInsightsStudio::OpenUSDInsightsStudio() : Studio() {
    _self = new OpenUSDInsightsStudio::data();
}

OpenUSDInsightsStudio::~OpenUSDInsightsStudio() {
    delete _self;
}

static const std::vector<Studio::ActivityConfig> activities = {
    {"Camera", true},
    {"Hydra", true},
    {"OpenUSD", true},
    {"Outliner", true},
    {"Usd Statistics", true},
    {"TfDebugger", false},
    {"UsdProperties", true},
};

const std::vector<Studio::ActivityConfig>& OpenUSDInsightsStudio::StudioConfiguration() const {
    return activities;
}

} // lab
