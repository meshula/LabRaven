
#include "CameraProvider.hpp"
#include "Lab/App.h"

#include <map>
#include <mutex>
#include <string>

namespace lab {

struct CameraProvider::data {
    data() {
        lookAts["##home"] = {
                                {{0, 5, 50},        // eye
                                {0, 0, 0},          // center
                                {0, 1, 0}},         // up
                                1
                            };
    }

    std::mutex safety;
    std::map<std::string, CameraData> lookAts;

    float currentLerpTime = 0;
    float lerpTime = 0;
    LookAt lerpStart;
    LookAt lerpEnd;
    std::string lerpName;
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

void CameraProvider::LerpLookAt(const LookAt& lookAt, float t, const std::string& name) {
    std::lock_guard<std::mutex> lock(_self->safety);
    auto it = _self->lookAts.find(name);
    if (it != _self->lookAts.end()) {
        _self->currentLerpTime = 0;
        _self->lerpTime = t;
        _self->lerpStart = it->second.lookAt;
        _self->lerpEnd = lookAt;
        _self->lerpName = name;
    }
}

inline lc_v3f operator+(lc_v3f a, lc_v3f b) {
    return lc_v3f{ a.x + b.x, a.y + b.y, a.z + b.z }; }
inline lc_v3f operator-(lc_v3f a, lc_v3f b) {
    return lc_v3f{ a.x - b.x, a.y - b.y, a.z - b.z }; }
inline lc_v3f operator*(lc_v3f a, float b) {
    return lc_v3f{ a.x * b, a.y * b, a.z * b }; }
inline lc_v3f operator/(lc_v3f a, float b) {
    return lc_v3f{ a.x / b, a.y / b, a.z / b }; }

void CameraProvider::Update(float dt) {
    if (_self->currentLerpTime < _self->lerpTime) {
        std::lock_guard<std::mutex> lock(_self->safety);
        auto it = _self->lookAts.find(_self->lerpName);
        if (it == _self->lookAts.end())
            return;

        _self->currentLerpTime += dt;
        float t = _self->currentLerpTime / _self->lerpTime;
        t = t > 1 ? 1 : t;
        if (t == 1) {
            _self->currentLerpTime = _self->lerpTime;
        }

        // cubic lerp
        t = t * t * (3 - 2 * t);

        it->second.lookAt.eye = _self->lerpStart.eye + (_self->lerpEnd.eye - _self->lerpStart.eye) * t;
        it->second.lookAt.center = _self->lerpStart.center + (_self->lerpEnd.center - _self->lerpStart.center) * t;
        it->second.lookAt.up = _self->lerpStart.up + (_self->lerpEnd.up - _self->lerpStart.up) * t;
        it->second.generation++;

        LabApp::instance()->SuspendPowerSave(10);
    }
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
    {
        std::lock_guard<std::mutex> lock(_self->safety);
        auto it = _self->lookAts.find(name);
        if (it != _self->lookAts.end()) {
            return it->second;
        }
    }
    return GetHome();
}

} // lab
