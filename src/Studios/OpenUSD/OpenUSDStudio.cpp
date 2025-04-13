//
//  Studios/TextureStudio.cpp
//  LabExcelsior
//
//  Created by Nick Porcino on 3/2/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#include "OpenUSDStudio.hpp"
#include <iostream>

namespace lab {
struct OpenUSDStudio::data {
};

OpenUSDStudio::OpenUSDStudio() : Studio() {
    _self = new OpenUSDStudio::data();
}

OpenUSDStudio::~OpenUSDStudio() {
    delete _self;
}

static const std::vector<Studio::ActivityConfig> activities = {
    {"Console", true},
    {"Component", true},
    {"Hydra", true},
    {"HydraOutliner", true},
    {"Journal", true},
    {"OpenUSD", true},
    {"UsdOutliner", true},
    {"Usd Statistics", true},
    {"Scout", true},
    {"TfDebugger", false},
    {"UsdProperties", true},
};

const std::vector<Studio::ActivityConfig>& OpenUSDStudio::StudioConfiguration() const {
    return activities;
}

} // lab
