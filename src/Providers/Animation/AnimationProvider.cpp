#include "AnimationProvider.hpp"
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/vec_float.h>
#include <ozz/base/maths/quaternion.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <cmath>

namespace lab {

// BVH parsing structures
struct BVHChannel {
    enum Type {
        Xposition, Yposition, Zposition,
        Xrotation, Yrotation, Zrotation
    };
    Type type;
    int index;  // Index in motion data
};

struct BVHJoint {
    std::string name;
    std::vector<BVHChannel> channels;
    std::vector<float> offset;
    std::vector<BVHJoint*> children;
    BVHJoint* parent;
    int index;  // Index in skeleton hierarchy
    
    BVHJoint() : parent(nullptr), index(-1) {}
    ~BVHJoint() {
        for (auto child : children) {
            delete child;
        }
    }
};

struct BVHData {
    BVHJoint* root;
    float frameTime;
    std::vector<std::vector<float>> frames;
    int totalChannels;
    
    BVHData() : root(nullptr), frameTime(0), totalChannels(0) {}
    ~BVHData() { delete root; }
};

// Helper functions for BVH parsing
static std::string ReadToken(std::istream& file) {
    std::string token;
    file >> token;
    return token;
}

static BVHJoint* ParseJoint(std::istream& file, BVHJoint* parent, int& channelOffset, int& jointIndex) {
    std::string token = ReadToken(file);
    if (token != "JOINT" && token != "End" && token != "ROOT") {
        return nullptr;
    }
    
    auto joint = new BVHJoint();
    joint->parent = parent;
    joint->index = jointIndex++;
    
    if (token != "End") {
        joint->name = ReadToken(file);
    } else {
        joint->name = "End Site";
        ReadToken(file);  // Skip "Site"
    }
    
    token = ReadToken(file);  // {
    if (token != "{") {
        delete joint;
        return nullptr;
    }
    
    // Read OFFSET
    token = ReadToken(file);
    if (token != "OFFSET") {
        delete joint;
        return nullptr;
    }
    
    joint->offset.resize(3);
    file >> joint->offset[0] >> joint->offset[1] >> joint->offset[2];
    
    // Read channels if not End Site
    if (joint->name != "End Site") {
        token = ReadToken(file);
        if (token != "CHANNELS") {
            delete joint;
            return nullptr;
        }
        
        int numChannels;
        file >> numChannels;
        
        joint->channels.resize(numChannels);
        for (int i = 0; i < numChannels; ++i) {
            token = ReadToken(file);
            BVHChannel channel;
            channel.index = channelOffset++;
            
            if (token == "Xposition") channel.type = BVHChannel::Xposition;
            else if (token == "Yposition") channel.type = BVHChannel::Yposition;
            else if (token == "Zposition") channel.type = BVHChannel::Zposition;
            else if (token == "Xrotation") channel.type = BVHChannel::Xrotation;
            else if (token == "Yrotation") channel.type = BVHChannel::Yrotation;
            else if (token == "Zrotation") channel.type = BVHChannel::Zrotation;
            
            joint->channels.push_back(channel);
        }
    }
    
    // Read child joints
    while (true) {
        token = ReadToken(file);
        if (token == "}") break;
        
        file.seekg(-token.length(), std::ios::cur);
        auto child = ParseJoint(file, joint, channelOffset, jointIndex);
        if (child) {
            joint->children.push_back(child);
        }
    }
    
    return joint;
}

static BVHData* ParseBVH(const char* filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return nullptr;
    }
    
    auto bvh = new BVHData();
    std::string token = ReadToken(file);
    if (token != "HIERARCHY") {
        delete bvh;
        return nullptr;
    }
    
    // Parse skeleton
    int channelOffset = 0;
    int jointIndex = 0;
    bvh->root = ParseJoint(file, nullptr, channelOffset, jointIndex);
    if (!bvh->root) {
        delete bvh;
        return nullptr;
    }
    
    bvh->totalChannels = channelOffset;
    
    // Parse motion
    token = ReadToken(file);
    if (token != "MOTION") {
        delete bvh;
        return nullptr;
    }
    
    token = ReadToken(file);
    if (token != "Frames:") {
        delete bvh;
        return nullptr;
    }
    
    int numFrames;
    file >> numFrames;
    
    token = ReadToken(file);
    if (token != "Frame") {
        delete bvh;
        return nullptr;
    }
    token = ReadToken(file);
    if (token != "Time:") {
        delete bvh;
        return nullptr;
    }
    
    file >> bvh->frameTime;
    
    // Read frame data
    bvh->frames.resize(numFrames);
    for (int i = 0; i < numFrames; ++i) {
        bvh->frames[i].resize(bvh->totalChannels);
        for (int j = 0; j < bvh->totalChannels; ++j) {
            file >> bvh->frames[i][j];
        }
    }
    
    return bvh;
}


struct AnimationProvider::data {
    // Skeleton data
    ozz::unique_ptr<ozz::animation::Skeleton> skeleton;

    // Animation data
    ozz::unique_ptr<ozz::animation::Animation> animation;
    
    // Runtime data
    float currentTime = 0.0f;
    bool playing = false;
    
    // Sampling buffers
    std::vector<ozz::math::SoaTransform> locals;
    std::vector<ozz::math::Float4x4> models;
    
    // Cache
    std::vector<float> joint_weights;
};

AnimationProvider::AnimationProvider() : Provider(AnimationProvider::sname()) {
    _self = new AnimationProvider::data();
    _instance = this;
    provider.Documentation = [](void* instance) -> const char* {
        return "Provides skeletal animation support using ozz-animation";
    };
}

AnimationProvider::~AnimationProvider() {
    UnloadAnimation();
    UnloadSkeleton();
    delete _self;
}

AnimationProvider* AnimationProvider::_instance = nullptr;
AnimationProvider* AnimationProvider::instance() {
    if (!_instance)
        new AnimationProvider();
    return _instance;
}

bool AnimationProvider::LoadSkeleton(const char* filename) {
    UnloadSkeleton(); // Clean up existing skeleton if any
    
    ozz::io::File file(filename, "rb");
    if (!file.opened()) {
        return false;
    }
    
    ozz::io::IArchive archive(&file);
    _self->skeleton = ozz::make_unique<ozz::animation::Skeleton>();
    if (!archive.TestTag<ozz::animation::Skeleton>()) {
        return false;
    }
    
    archive >> *_self->skeleton;
    
    // Initialize buffers
    const int num_joints = _self->skeleton->num_joints();
    const int num_soa_joints = (num_joints + 3) / 4;
    _self->locals.resize(num_soa_joints);
    _self->models.resize(num_joints);
    _self->joint_weights.resize(num_joints, 1.f);
    
    return true;
}

void AnimationProvider::UnloadSkeleton() {
    _self->skeleton.reset();
    _self->locals.clear();
    _self->models.clear();
    _self->joint_weights.clear();
}

bool AnimationProvider::HasSkeleton() const {
    return _self->skeleton != nullptr;
}

bool AnimationProvider::LoadAnimation(const char* filename) {
    UnloadAnimation(); // Clean up existing animation if any
    
    ozz::io::File file(filename, "rb");
    if (!file.opened()) {
        return false;
    }
    
    ozz::io::IArchive archive(&file);
    _self->animation = ozz::make_unique<ozz::animation::Animation>();
    if (!archive.TestTag<ozz::animation::Animation>()) {
        return false;
    }
    
    archive >> *_self->animation;
    return true;
}

void AnimationProvider::UnloadAnimation() {
    _self->animation.reset();
    _self->currentTime = 0.0f;
    _self->playing = false;
}

bool AnimationProvider::HasAnimation() const {
    return _self->animation != nullptr;
}

float AnimationProvider::CurrentTime() const {
    return _self->currentTime;
}

void AnimationProvider::SetCurrentTime(float time) {
    if (_self->animation) {
        _self->currentTime = std::fmod(time, _self->animation->duration());
    }
}

float AnimationProvider::Duration() const {
    return _self->animation ? _self->animation->duration() : 0.0f;
}

bool AnimationProvider::IsPlaying() const {
    return _self->playing;
}

void AnimationProvider::SetPlaying(bool playing) {
    _self->playing = playing;
}

void AnimationProvider::Update(float deltaTime) {
    if (!_self->animation || !_self->skeleton || !_self->playing) {
        return;
    }
    
    // Update animation time
    _self->currentTime = std::fmod(_self->currentTime + deltaTime, _self->animation->duration());
    
    // Setup sampling job
    ozz::animation::SamplingJob sampling_job;
    sampling_job.animation = _self->animation.get();
    sampling_job.ratio = _self->currentTime / _self->animation->duration();
    sampling_job.output = ozz::make_span(_self->locals);
    
    // Sample animation
    if (!sampling_job.Run()) {
        return;
    }
    
    // Convert local transforms to model-space matrices
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = _self->skeleton.get();
    ltm_job.input = ozz::make_span(_self->locals);
    ltm_job.output = ozz::make_span(_self->models);
    
    if (!ltm_job.Run()) {
        return;
    }
}

const ozz::math::Float4x4* AnimationProvider::GetJointTransforms() const {
    return _self->models.data();
}

int AnimationProvider::GetJointCount() const {
    return _self->skeleton ? _self->skeleton->num_joints() : 0;
}

bool AnimationProvider::LoadSkeletonFromBVH(const char* filename) {
    std::unique_ptr<BVHData> bvh(ParseBVH(filename));
    if (!bvh || !bvh->root) {
        return false;
    }

    UnloadSkeleton();

    // Count joints
    std::function<int(BVHJoint*)> countJoints = [&](BVHJoint* joint) {
        int count = 1;  // Count this joint
        for (auto child : joint->children) {
            count += countJoints(child);
        }
        return count;
    };
    int numJoints = countJoints(bvh->root);

    // Create raw skeleton
    ozz::animation::offline::RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(1);
    
    // Build joint hierarchy
    std::function<void(BVHJoint*, ozz::animation::offline::RawSkeleton::Joint&)> buildHierarchy = 
        [&](BVHJoint* joint, ozz::animation::offline::RawSkeleton::Joint& raw_joint) {
            raw_joint.name = joint->name;
            raw_joint.transform.translation = ozz::math::Float3(
                joint->offset[0],
                joint->offset[1],
                joint->offset[2]
            );
            raw_joint.transform.rotation = ozz::math::Quaternion::identity();
            raw_joint.transform.scale = ozz::math::Float3(1.f);
            
            raw_joint.children.resize(joint->children.size());
            for (size_t i = 0; i < joint->children.size(); ++i) {
                buildHierarchy(joint->children[i], raw_joint.children[i]);
            }
        };
    
    buildHierarchy(bvh->root, raw_skeleton.roots[0]);

    // Build runtime skeleton
    ozz::animation::offline::SkeletonBuilder builder;
    _self->skeleton = builder(raw_skeleton);
    if (!_self->skeleton) {
        return false;
    }

    // Initialize buffers
    const int num_joints = _self->skeleton->num_joints();
    const int num_soa_joints = (num_joints + 3) / 4;
    _self->locals.resize(num_soa_joints);
    _self->models.resize(num_joints);
    _self->joint_weights.resize(num_joints, 1.f);

    return true;
}

bool AnimationProvider::LoadAnimationFromBVH(const char* filename) {
    std::unique_ptr<BVHData> bvh(ParseBVH(filename));
    if (!bvh || !bvh->root || bvh->frames.empty()) {
        return false;
    }

    UnloadAnimation();

    // Create raw animation
    ozz::animation::offline::RawAnimation raw_animation;
    raw_animation.duration = bvh->frameTime * bvh->frames.size();
    raw_animation.tracks.resize(GetJointCount());

    // Map joints to their indices
    std::map<std::string, int> joint_indices;
    std::function<void(BVHJoint*)> mapJointIndices = [&](BVHJoint* joint) {
        joint_indices[joint->name] = joint->index;
        for (auto child : joint->children) {
            mapJointIndices(child);
        }
    };
    mapJointIndices(bvh->root);

    // Create track for each joint
    std::function<void(BVHJoint*)> createTracks = [&](BVHJoint* joint) {
        auto& track = raw_animation.tracks[joint->index];
        
        // Add keyframes for each frame
        for (size_t i = 0; i < bvh->frames.size(); ++i) {
            ozz::math::Float3 translation(0.f);
            ozz::math::Float3 rotation(0.f);
            
            // Process channels
            for (const auto& channel : joint->channels) {
                float value = bvh->frames[i][channel.index];
                switch (channel.type) {
                    case BVHChannel::Xposition: translation.x = value; break;
                    case BVHChannel::Yposition: translation.y = value; break;
                    case BVHChannel::Zposition: translation.z = value; break;
                    case BVHChannel::Xrotation: rotation.x = value * 0.0174533f; break;
                    case BVHChannel::Yrotation: rotation.y = value * 0.0174533f; break;
                    case BVHChannel::Zrotation: rotation.z = value * 0.0174533f; break;
                }
            }
            
            // Convert Euler angles to quaternion
            ozz::math::Quaternion quat = ozz::math::Quaternion::FromEuler(rotation);
            
            // Add keyframe
            float time = i * bvh->frameTime;
            track.translations.push_back({time, translation});
            track.rotations.push_back({time, quat});
            track.scales.push_back({time, ozz::math::Float3(1.f)});
        }
        
        // Process children
        for (auto child : joint->children) {
            createTracks(child);
        }
    };
    
    createTracks(bvh->root);

    // Build runtime animation
    ozz::animation::offline::AnimationBuilder builder;
    _self->animation = builder(raw_animation);
    if (!_self->animation) {
        return false;
    }

    return true;
}

} // lab
