//
//  Studios/TextureStudio.cpp
//  LabExcelsior
//
//  Created by Nick Porcino on 3/2/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#include "TextureStudio.hpp"
#include <iostream>

namespace lab {
struct TextureStudio::data {
};

TextureStudio::TextureStudio() : Studio() {
    _self = new TextureStudio::data();
}

TextureStudio::~TextureStudio() {
    delete _self;
}

static const std::vector<Studio::ActivityConfig> activities = {
    {"Color", true},
    {"Texture", true}
};

const std::vector<Studio::ActivityConfig>& TextureStudio::StudioConfiguration() const {
    return activities;
}

} // lab
