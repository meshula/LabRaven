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
    bool LoadSkeleton(const char* filename);
    bool LoadSkeletonFromBVH(const char* filename);
    void UnloadSkeleton();
    bool HasSkeleton() const;

    // Animation management
    bool LoadAnimation(const char* filename);
    bool LoadAnimationFromBVH(const char* filename);
    void UnloadAnimation();
    bool HasAnimation() const;

    // Playback control
    float CurrentTime() const;
    void SetCurrentTime(float time);
    float Duration() const;
    
    bool IsPlaying() const;
    void SetPlaying(bool playing);

    // Sampling
    void Update(float deltaTime);
    const ozz::math::Float4x4* GetJointTransforms() const;
    int GetJointCount() const;
};

} // lab
#endif // Providers_Animation_AnimationProvider_hpp
