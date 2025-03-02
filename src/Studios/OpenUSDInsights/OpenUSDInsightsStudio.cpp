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

static const std::vector<std::string> activities = {
    "Camera",
    "Hydra",
    "OpenUSD",
    "Outliner",
    "Usd Statistics",
    "TfDebugger",
    "UsdProperties",
};

const std::vector<std::string>& OpenUSDInsightsStudio::StudioConfiguration() const {
    return activities;
}

} // lab
