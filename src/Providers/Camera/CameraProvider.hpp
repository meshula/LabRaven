#ifndef Providers_Camera_hpp
#define Providers_Camera_hpp

#include "Lab/StudioCore.hpp"
#include "LabCamera.h"

namespace lab {

class CameraProvider : public Provider {
    struct data;
    data* _self;
    static CameraProvider* _instance;
public:
    CameraProvider();
    ~CameraProvider();

    static CameraProvider* instance();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Camera"; }

    struct LookAt {
        lc_v3f eye;
        lc_v3f center;
        lc_v3f up;
    };
    struct CameraData {
        CameraProvider::LookAt lookAt;
        lc_radians HFOV = { 0.7f }; // 40 degrees, approximately
        int generation = 0;
    };

    void SetLookAt(const LookAt& lookAt, const std::string& name);
    CameraData GetLookAt(const std::string& name) const;
    void SetHFOV(lc_radians hfov, const std::string& name);

    void LerpLookAt(const LookAt& lookAt, float t, const std::string& name);
    void Update(float dt);

    void SetHome(const LookAt& lookAt) { SetLookAt(lookAt, "##home"); }
    CameraData GetHome() const { return GetLookAt("##home"); }
};

} // lab
#endif // Providers_Camera_hpp
