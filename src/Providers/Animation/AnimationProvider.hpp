#ifndef Providers_Animation_AnimationProvider_hpp
#define Providers_Animation_AnimationProvider_hpp

#include "Lab/StudioCore.hpp"
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <string>
#include <memory>

namespace lab {

class AnimationProvider : public Provider {
    struct data;
    data* _self;
    static AnimationProvider* _instance;
public:
    AnimationProvider();
    ~AnimationProvider();

    static AnimationProvider* instance();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Animation"; }

    // Skeleton management
    bool LoadSkeleton(const char* name, const char* filename);
    bool LoadSkeletonFromBVH(const char* name, const char* filename);
    void UnloadSkeleton(const char* name);
    void UnloadAllSkeletons();
    bool HasSkeleton(const char* name) const;
    const ozz::animation::Skeleton* GetSkeleton(const char* name) const;
    std::vector<std::string> GetSkeletonNames() const;

    // Animation management
    bool LoadAnimation(const char* name, const char* filename);
    bool LoadAnimationFromBVH(const char* name, const char* filename, const char* skeletonName);
    void UnloadAnimation(const char* name);
    void UnloadAllAnimations();
    bool HasAnimation(const char* name) const;
    const ozz::animation::Animation* GetAnimation(const char* name) const;
    std::vector<std::string> GetAnimationNames() const;

    // Instance management
    struct Instance {
        std::string skeletonName;
        std::string animationName;
        float currentTime = 0.0f;
        bool playing = false;
        std::vector<ozz::math::SoaTransform> locals;
        std::vector<ozz::math::Float4x4> models;
        std::vector<float> jointWeights;
    };

    Instance* CreateInstance(const char* name, const char* skeletonName, const char* animationName);
    void DestroyInstance(const char* name);
    Instance* GetInstance(const char* name);
    const Instance* GetInstance(const char* name) const;
    std::vector<std::string> GetInstanceNames() const;

    // Instance control
    float GetCurrentTime(const char* instanceName) const;
    void SetCurrentTime(const char* instanceName, float time);
    float GetDuration(const char* instanceName) const;
    bool IsPlaying(const char* instanceName) const;
    void SetPlaying(const char* instanceName, bool playing);

    // Instance sampling
    void Update(float deltaTime);
    const ozz::math::Float4x4* GetJointTransforms(const char* instanceName) const;
    int GetJointCount(const char* instanceName) const;
};

} // lab
#endif // Providers_Animation_AnimationProvider_hpp
