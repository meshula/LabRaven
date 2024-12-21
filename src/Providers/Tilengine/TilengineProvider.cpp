
#include "TilengineProvider.hpp"

namespace lab {

    struct TilengineProvider::data {
        data() = default;
        ~data() = default;
    };
    
    TilengineProvider::TilengineProvider() : Provider(TilengineProvider::sname()) {
        _self = new TilengineProvider::data;
    }

    TilengineProvider::~TilengineProvider() {
        delete _self;
    }

    TilengineProvider* TilengineProvider::instance() {
        if (!_instance) {
            _instance = new TilengineProvider();
        }
        return _instance;
    }

    TilengineProvider* TilengineProvider::_instance = nullptr;
} // lab
