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
    bool restoreTm = false;
};

TextureStudio::TextureStudio() : Studio() {
    _self = new TextureStudio::data();
}

TextureStudio::~TextureStudio() {
    delete _self;
}

static const std::vector<std::string> modes = {
    "Color",
    "Texture",
};

const std::vector<std::string>& TextureStudio::StudioConfiguration() const {
    return modes;
}

} // lab
