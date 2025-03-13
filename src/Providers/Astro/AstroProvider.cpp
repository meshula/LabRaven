
#include "AstroProvider.hpp"
#include <set>
#include <string>

namespace lab {

struct AstroProvider::data {
};

AstroProvider::AstroProvider() : Provider(AstroProvider::sname()) {
    _self = new AstroProvider::data();
    _instance = this;
    provider.Documentation = [](void* instance) -> const char* {
        return "Provides astronomy related functionality";
    };
}

AstroProvider::~AstroProvider() {
    delete _self;
}

AstroProvider* AstroProvider::_instance = nullptr;
AstroProvider* AstroProvider::instance() {
    if (!_instance)
        new AstroProvider();
    return _instance;
}

} // lab
