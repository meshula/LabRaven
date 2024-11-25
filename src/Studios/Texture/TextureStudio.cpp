//
//  Studios/TextureStudio.cpp
//  LabExcelsior
//
//  Created by Nick Porcino on 3/2/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#include "TextureStudio.hpp"
#include "Activities/Color/ColorActivity.hpp"
//#include "Activities/Texture/TextureActivity.hpp"
#include <iostream>

namespace lab {
struct TextureStudio::data {
    std::weak_ptr<lab::ColorActivity> colourMM;
 //   std::weak_ptr<lab::TextureActivity> textureMM;
    bool restoreTm = false;
};

TextureStudio::TextureStudio() : Studio() {
    _self = new TextureStudio::data();
}

TextureStudio::~TextureStudio() {
    delete _self;
}

static const std::vector<std::string> modes = {
    "Colour",
    "Texture",
};

const std::vector<std::string>& TextureStudio::StudioConfiguration() const {
    return modes;
}

} // lab
