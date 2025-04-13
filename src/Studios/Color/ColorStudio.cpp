//
//  Studios/TextureStudio.cpp
//  LabExcelsior
//
//  Created by Nick Porcino on 15/12/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#include "ColorStudio.hpp"
#include <iostream>

namespace lab {
struct ColorStudio::data {
};

ColorStudio::ColorStudio() : Studio() {
    _self = new ColorStudio::data();
}

ColorStudio::~ColorStudio() {
    delete _self;
}

static const std::vector<Studio::ActivityConfig> activities = {
    {"Color", true}
};

const std::vector<Studio::ActivityConfig>& ColorStudio::StudioConfiguration() const {
    return activities;
}

} // lab
