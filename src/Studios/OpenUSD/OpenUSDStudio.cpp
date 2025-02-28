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

static const std::vector<std::string> activities = {
    "Console",
    "Hydra",
    "Journal",
    "OpenUSD",
    "Outliner",
    "TfDebugger",
    "UsdProperties",
};

const std::vector<std::string>& OpenUSDStudio::StudioConfiguration() const {
    return activities;
}

} // lab
