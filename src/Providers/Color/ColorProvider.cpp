
#include "ColorProvider.hpp"
#include <set>
#include <string>

namespace lab {

struct ColorProvider::data {
    std::set<std::string> colorSpaces;
    std::string renderingColorSpace;
};

ColorProvider::ColorProvider() {
    _self = new ColorProvider::data();
    _instance = this;
}

ColorProvider::~ColorProvider() {
    delete _self;
}

ColorProvider* ColorProvider::_instance = nullptr;
ColorProvider* ColorProvider::instance() {
    if (!_instance)
        new ColorProvider();
    return _instance;
}

const char* ColorProvider::RenderingColorSpace() const {
    // return the persistent string from the colorSpaces name cache
    auto i = _self->colorSpaces.find(_self->renderingColorSpace);
    if (i != _self->colorSpaces.end())
        return _self->renderingColorSpace.c_str();
    return nullptr;
}

void ColorProvider::SetRenderingColorSpace(const char* colorSpace) {
    _self->renderingColorSpace = colorSpace;
    _self->colorSpaces.insert(colorSpace);
}

} // lab
