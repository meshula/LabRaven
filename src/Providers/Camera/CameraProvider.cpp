
#include "CameraProvider.hpp"

#include <map>
#include <mutex>
#include <string>

namespace lab {

struct CameraProvider::data {
    data() {
        lookAts["##home"] = {
                                {{0, 0, 50},        // eye
                                {0, 0, 0},          // center
                                {0, 1, 0}},         // up
                                1
                            };
    }

    std::mutex safety;
    std::map<std::string, CameraData> lookAts;
};

CameraProvider::CameraProvider() : Provider(CameraProvider::sname()) {
    _self = new CameraProvider::data();
    _instance = this;
    provider.Documentation = [](void* instance) -> const char* {
        return "Maintains camera settings";
    };
}

CameraProvider::~CameraProvider() {
    delete _self;
}

CameraProvider* CameraProvider::_instance = nullptr;
CameraProvider* CameraProvider::instance() {
    if (!_instance)
        new CameraProvider();
    return _instance;
}

void CameraProvider::SetLookAt(const LookAt& lookAt, const std::string& name) {
    std::lock_guard<std::mutex> lock(_self->safety);
    auto it = _self->lookAts.find(name);
    if (it != _self->lookAts.end()) {
        it->second.lookAt = lookAt;
        it->second.generation++;
    }
    else {
        _self->lookAts[name] = { lookAt, 1 };
    }
}

CameraProvider::CameraData CameraProvider::GetLookAt(const std::string& name) const {
    std::lock_guard<std::mutex> lock(_self->safety);
    if (_self->lookAts.find(name) == _self->lookAts.end())
        return GetHome();
    return _self->lookAts[name];
}

} // lab
