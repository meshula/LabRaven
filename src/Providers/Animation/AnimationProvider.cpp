#include "AnimationProvider.hpp"
#include "LabBVH.hpp"
#include <ozz/animation/runtime/skeleton_utils.h>
#define CGLTF_IMPLEMENTATION
#include <cgltf/cgltf.h>
#define CGLTF_VRM_IMPLEMENTATION
#include <cgltf_vrm/cgltf_vrm.h>
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

struct AnimationProvider::data {
    // Resource management
    std::map<std::string, ozz::unique_ptr<ozz::animation::Skeleton>> skeletons;
    std::map<std::string, ozz::unique_ptr<ozz::animation::Animation>> animations;
    std::map<std::string, std::unique_ptr<Instance>> instances;
    std::map<std::string, std::unique_ptr<Model>> models;
};

AnimationProvider::AnimationProvider() : Provider(AnimationProvider::sname()) {
    _self = new AnimationProvider::data();
    _instance = this;
    provider.Documentation = [](void* instance) -> const char* {
        return "Provides skeletal animation support using ozz-animation";
    };
}

AnimationProvider::~AnimationProvider() {
    UnloadAllAnimations();
    UnloadAllSkeletons();
    delete _self;
}

AnimationProvider* AnimationProvider::_instance = nullptr;
AnimationProvider* AnimationProvider::instance() {
    if (!_instance)
        new AnimationProvider();
    return _instance;
}

bool AnimationProvider::LoadSkeleton(const char* name, const char* filename) {
    ozz::io::File file(filename, "rb");
    if (!file.opened()) {
        return false;
    }
    
    ozz::io::IArchive archive(&file);
    auto skeleton = ozz::make_unique<ozz::animation::Skeleton>();
    if (!archive.TestTag<ozz::animation::Skeleton>()) {
        return false;
    }
    
    archive >> *skeleton;
    _self->skeletons[name] = std::move(skeleton);
    return true;
}

void AnimationProvider::UnloadSkeleton(const char* name) {
    _self->skeletons.erase(name);
    
    // Clean up any instances using this skeleton
    for (auto it = _self->instances.begin(); it != _self->instances.end();) {
        if (it->second->skeletonName == name) {
            it = _self->instances.erase(it);
        } else {
            ++it;
        }
    }
}

void AnimationProvider::UnloadAllSkeletons() {
    _self->skeletons.clear();
    _self->instances.clear(); // All instances are invalid without skeletons
}

bool AnimationProvider::HasSkeleton(const char* name) const {
    return _self->skeletons.find(name) != _self->skeletons.end();
}

const ozz::animation::Skeleton* AnimationProvider::GetSkeleton(const char* name) const {
    auto it = _self->skeletons.find(name);
    return it != _self->skeletons.end() ? it->second.get() : nullptr;
}

std::vector<std::string> AnimationProvider::GetSkeletonNames() const {
    std::vector<std::string> names;
    names.reserve(_self->skeletons.size());
    for (const auto& pair : _self->skeletons) {
        names.push_back(pair.first);
    }
    return names;
}

bool AnimationProvider::LoadAnimation(const char* name, const char* filename) {
    ozz::io::File file(filename, "rb");
    if (!file.opened()) {
        return false;
    }
    
    ozz::io::IArchive archive(&file);
    auto animation = ozz::make_unique<ozz::animation::Animation>();
    if (!archive.TestTag<ozz::animation::Animation>()) {
        return false;
    }
    
    archive >> *animation;
    _self->animations[name] = std::move(animation);
    return true;
}

void AnimationProvider::UnloadAnimation(const char* name) {
    _self->animations.erase(name);
    
    // Clean up any instances using this animation
    for (auto it = _self->instances.begin(); it != _self->instances.end();) {
        if (it->second->animationName == name) {
            it = _self->instances.erase(it);
        } else {
            ++it;
        }
    }
}

void AnimationProvider::UnloadAllAnimations() {
    _self->animations.clear();
    _self->instances.clear(); // All instances are invalid without animations
}


bool AnimationProvider::LoadSkeletonFromBVH(const char* name, const char* filename) {
    const bool printDiagnostics = true;

    std::unique_ptr<BVHData> bvh(lab::ParseBVH(filename));
    if (!bvh || !bvh->root) {
        return false;
    }

    UnloadSkeleton(name);

    if (printDiagnostics) {
        printf("Loading skeleton from BVH file: %s\n", filename);
        printf("Skeleton hierarchy:\n");

        std::function<void(BVHJoint*, int)> printJoint = [&](BVHJoint* joint, int depth) {
            // Print indentation
            for (int i = 0; i < depth; i++) {
                printf("  ");
            }

            // Print joint info
            printf("- %s (index: %d, offset: %.2f, %.2f, %.2f)\n",
                joint->name.c_str(),
                joint->index,
                joint->offset[0],
                joint->offset[1],
                joint->offset[2]
            );

            // Print channels
            if (!joint->channels.empty()) {
                for (int i = 0; i < depth + 1; i++) printf("  ");
                printf("channels: ");
                for (const auto& channel : joint->channels) {
                    const char* type = "";
                    switch (channel.type) {
                        case BVHChannel::Xposition: type = "Xpos"; break;
                        case BVHChannel::Yposition: type = "Ypos"; break;
                        case BVHChannel::Zposition: type = "Zpos"; break;
                        case BVHChannel::Xrotation: type = "Xrot"; break;
                        case BVHChannel::Yrotation: type = "Yrot"; break;
                        case BVHChannel::Zrotation: type = "Zrot"; break;
                    }
                    printf("%s(%d) ", type, channel.index);
                }
                printf("\n");
            }

            // Recursively print children
            for (auto child : joint->children) {
                printJoint(child, depth + 1);
            }
        };

        printJoint(bvh->root, 0);
        printf("\n");
    }

    // Count joints
    std::function<int(BVHJoint*)> countJoints = [&](BVHJoint* joint) {
        int count = 1;  // Count this joint
        for (auto child : joint->children) {
            count += countJoints(child);
        }
        return count;
    };
    int numJoints = countJoints(bvh->root);

    // Create an instance to store the rest pose
    auto instance = std::make_unique<Instance>();
    instance->skeletonName = name;
    instance->animationName = "";

    // Initialize buffers
    instance->locals.resize(numJoints);
    instance->models.resize(numJoints);
    instance->jointWeights.resize(numJoints, 1.f);

    // Initialize all transforms to identity
    for (int i = 0; i < numJoints; ++i) {
        instance->locals[i] = ozz::math::SoaTransform::identity();
        instance->models[i] = ozz::math::Float4x4::identity();
    }

    // Create raw skeleton
    ozz::animation::offline::RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(1);
    ozz::animation::offline::RawSkeleton::Joint& root = raw_skeleton.roots[0];
    root.name = bvh->root->name;
    root.transform.translation = ozz::math::Float3(bvh->root->offset[0], bvh->root->offset[1], bvh->root->offset[2]);
    root.transform.rotation = ozz::math::Quaternion::identity();
    root.transform.scale = ozz::math::Float3(1.f);
    root.children.resize(bvh->root->children.size());

    // Build joint hierarchy
    int currentJoint = 0;
    std::function<void(BVHJoint*, ozz::animation::offline::RawSkeleton::Joint&)> buildHierarchy =
        [&](BVHJoint* joint, ozz::animation::offline::RawSkeleton::Joint& raw_joint) {
            raw_joint.name = joint->name;
            raw_joint.transform.translation = ozz::math::Float3(
                joint->offset[0],
                joint->offset[1],
                joint->offset[2]
            );

            printf("%f %f %f\n", joint->offset[0], joint->offset[1], joint->offset[2]);

            raw_joint.transform.rotation = ozz::math::Quaternion::identity();
            raw_joint.transform.scale = ozz::math::Float3(1.f);

            instance->locals[currentJoint].translation = { joint->offset[0],
                joint->offset[1],
                joint->offset[2] };
            instance->locals[currentJoint].rotation = ozz::math::SoaQuaternion::identity();
            instance->locals[currentJoint].scale = {1,1,1};

            ++currentJoint;

            raw_joint.children.resize(joint->children.size());
            for (size_t i = 0; i < joint->children.size(); ++i) {
                buildHierarchy(joint->children[i], raw_joint.children[i]);
            }
        };

    buildHierarchy(bvh->root, root);

    // Validate skeleton
    if (!raw_skeleton.Validate()) {
        printf("Invalid skeleton\n");
        return false;
    }

    // Build runtime skeleton
    ozz::animation::offline::SkeletonBuilder builder;
    auto skeleton = builder(raw_skeleton);
    if (!skeleton) {
        return false;
    }

    // Convert to model space
    ozz::animation::LocalToModelJob ltm_job;
    ltm_job.skeleton = skeleton.get();
    ltm_job.input = ozz::make_span(instance->locals);
    ltm_job.output = ozz::make_span(instance->models);
    ltm_job.Run();

    // Store skeleton and instance
    _self->skeletons[name] = std::move(skeleton);
    _self->instances[name] = std::move(instance);
    return true;
}

bool AnimationProvider::LoadSkeletonFromGLTF(const char* name, const char* filename) {
    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, filename, &data);
    if (result != cgltf_result_success) {
        return false;
    }

    result = cgltf_load_buffers(&options, data, filename);
    if (result != cgltf_result_success) {
        cgltf_free(data);
        return false;
    }

    // Create raw skeleton
    ozz::animation::offline::RawSkeleton raw_skeleton;
    raw_skeleton.roots.resize(1);

    // Find the first node with a skin
    cgltf_node* root_node = nullptr;
    cgltf_skin* skin = nullptr;
    for (size_t i = 0; i < data->nodes_count; i++) {
        if (data->nodes[i].skin) {
            root_node = &data->nodes[i];
            skin = data->nodes[i].skin;
            break;
        }
    }

    if (!root_node || !skin) {
        cgltf_free(data);
        return false;
    }

    // Build joint hierarchy
    std::function<void(cgltf_node*, ozz::animation::offline::RawSkeleton::Joint&)> buildJoint =
        [&](cgltf_node* node, ozz::animation::offline::RawSkeleton::Joint& joint) {
            joint.name = node->name ? node->name : "joint";
            
            // Get transform
            if (node->has_translation) {
                joint.transform.translation = ozz::math::Float3(
                    node->translation[0],
                    node->translation[1],
                    node->translation[2]
                );
            }
            
            if (node->has_rotation) {
                joint.transform.rotation = ozz::math::Quaternion(
                    node->rotation[0],
                    node->rotation[1],
                    node->rotation[2],
                    node->rotation[3]
                );
            }
            
            if (node->has_scale) {
                joint.transform.scale = ozz::math::Float3(
                    node->scale[0],
                    node->scale[1],
                    node->scale[2]
                );
            }

            // Process children
            joint.children.resize(node->children_count);
            for (size_t i = 0; i < node->children_count; i++) {
                buildJoint(node->children[i], joint.children[i]);
            }
        };

    // Build the skeleton starting from root
    buildJoint(root_node, raw_skeleton.roots[0]);

    // Validate skeleton
    if (!raw_skeleton.Validate()) {
        cgltf_free(data);
        return false;
    }

    // Build runtime skeleton
    ozz::animation::offline::SkeletonBuilder builder;
    auto skeleton = builder(raw_skeleton);
    if (!skeleton) {
        cgltf_free(data);
        return false;
    }

    // Store skeleton
    _self->skeletons[name] = std::move(skeleton);

    // Create instance for rest pose
    auto instance = std::make_unique<Instance>();
    instance->skeletonName = name;
    instance->animationName = "";

    // Initialize buffers
    const int num_joints = _self->skeletons[name]->num_joints();
    const int num_soa_joints = (num_joints + 3) / 4;
    instance->locals.resize(num_soa_joints);
    instance->models.resize(num_joints);
    instance->jointWeights.resize(num_joints, 1.f);

    // Initialize transforms
    for (int i = 0; i < num_soa_joints; ++i) {
        instance->locals[i] = ozz::math::SoaTransform::identity();
    }
    for (int i = 0; i < num_joints; ++i) {
        instance->models[i] = ozz::math::Float4x4::identity();
    }

    // Store instance
    _self->instances[name] = std::move(instance);

    cgltf_free(data);
    return true;
}

bool AnimationProvider::LoadSkeletonFromVRM(const char* name, const char* filename) {
    // VRM is an extension of GLTF, so we can use the same loading code
    return LoadSkeletonFromGLTF(name, filename);
}

bool AnimationProvider::LoadAnimationFromGLTF(const char* name, const char* filename, const char* skeletonName) {
    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, filename, &data);
    if (result != cgltf_result_success) {
        return false;
    }

    result = cgltf_load_buffers(&options, data, filename);
    if (result != cgltf_result_success) {
        cgltf_free(data);
        return false;
    }

    // Get skeleton
    auto skeleton = GetSkeleton(skeletonName);
    if (!skeleton) {
        cgltf_free(data);
        return false;
    }

    // Create raw animation
    ozz::animation::offline::RawAnimation raw_animation;

    // Find animation data
    if (data->animations_count == 0) {
        cgltf_free(data);
        return false;
    }

    // Use first animation
    cgltf_animation* gltf_animation = &data->animations[0];
    
    // Calculate duration
    float max_time = 0.0f;
    for (size_t i = 0; i < gltf_animation->samplers_count; i++) {
        cgltf_animation_sampler* sampler = &gltf_animation->samplers[i];
        if (sampler->input->count > 0) {
            float* times = (float*)sampler->input->buffer_view->buffer->data + 
                         sampler->input->offset / sizeof(float);
            max_time = std::max(max_time, times[sampler->input->count - 1]);
        }
    }
    raw_animation.duration = max_time;

    // Initialize tracks
    raw_animation.tracks.resize(skeleton->num_joints());

    // Process animation channels
    for (size_t i = 0; i < gltf_animation->channels_count; i++) {
        cgltf_animation_channel* channel = &gltf_animation->channels[i];
        cgltf_animation_sampler* sampler = channel->sampler;
        cgltf_node* target_node = channel->target_node;

        // Find joint index
        int joint_index = -1;
        for (size_t j = 0; j < skeleton->num_joints(); j++) {
            if (strcmp(skeleton->joint_names()[j], target_node->name) == 0) {
                joint_index = j;
                break;
            }
        }
        if (joint_index == -1) continue;

        // Get keyframe times
        float* times = (float*)sampler->input->buffer_view->buffer->data + 
                     sampler->input->offset / sizeof(float);
        size_t time_stride = sampler->input->stride / sizeof(float);

        // Get keyframe values
        float* values = (float*)sampler->output->buffer_view->buffer->data + 
                      sampler->output->offset / sizeof(float);
        size_t value_stride = sampler->output->stride / sizeof(float);

        // Add keyframes
        auto& track = raw_animation.tracks[joint_index];
        for (size_t j = 0; j < sampler->input->count; j++) {
            float time = times[j * time_stride];
            
            switch (channel->target_path) {
                case cgltf_animation_path_type_translation: {
                    ozz::math::Float3 translation(
                        values[j * value_stride],
                        values[j * value_stride + 1],
                        values[j * value_stride + 2]
                    );
                    track.translations.push_back({time, translation});
                    break;
                }
                case cgltf_animation_path_type_rotation: {
                    ozz::math::Quaternion rotation(
                        values[j * value_stride],
                        values[j * value_stride + 1],
                        values[j * value_stride + 2],
                        values[j * value_stride + 3]
                    );
                    track.rotations.push_back({time, rotation});
                    break;
                }
                case cgltf_animation_path_type_scale: {
                    ozz::math::Float3 scale(
                        values[j * value_stride],
                        values[j * value_stride + 1],
                        values[j * value_stride + 2]
                    );
                    track.scales.push_back({time, scale});
                    break;
                }
                default:
                    break;
            }
        }
    }

    // Build runtime animation
    ozz::animation::offline::AnimationBuilder builder;
    auto animation = builder(raw_animation);
    if (!animation) {
        cgltf_free(data);
        return false;
    }

    // Store animation
    _self->animations[name] = std::move(animation);

    cgltf_free(data);
    return true;
}

bool AnimationProvider::LoadAnimationFromVRM(const char* name, const char* filename, const char* skeletonName) {
    // VRM is an extension of GLTF, so we can use the same loading code
    return LoadAnimationFromGLTF(name, filename, skeletonName);
}

bool AnimationProvider::LoadModelFromGLTF(const char* name, const char* filename, const char* skeletonName) {
    // Verify skeleton exists
    if (!HasSkeleton(skeletonName)) {
        return false;
    }

    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, filename, &data);
    if (result != cgltf_result_success) {
        return false;
    }

    result = cgltf_load_buffers(&options, data, filename);
    if (result != cgltf_result_success) {
        cgltf_free(data);
        return false;
    }

    // Create model
    auto model = std::make_unique<Model>();
    model->name = name;
    model->skeletonName = skeletonName;

    // Process each mesh
    for (size_t i = 0; i < data->meshes_count; i++) {
        const cgltf_mesh& gltf_mesh = data->meshes[i];
        
        // Create mesh
        auto mesh = std::make_shared<Mesh>();
        mesh->name = gltf_mesh.name ? gltf_mesh.name : "mesh";

        // Process each primitive
        for (size_t j = 0; j < gltf_mesh.primitives_count; j++) {
            const cgltf_primitive& primitive = gltf_mesh.primitives[j];

            // Get vertex count
            size_t vertex_count = 0;
            for (size_t k = 0; k < primitive.attributes_count; k++) {
                const cgltf_attribute& attribute = primitive.attributes[k];
                if (attribute.type == cgltf_attribute_type_position) {
                    vertex_count = attribute.data->count;
                    break;
                }
            }

            if (vertex_count == 0) continue;

            // Process attributes
            for (size_t k = 0; k < primitive.attributes_count; k++) {
                const cgltf_attribute& attribute = primitive.attributes[k];
                const cgltf_accessor* accessor = attribute.data;
                
                switch (attribute.type) {
                    case cgltf_attribute_type_position: {
                        mesh->positions.resize(vertex_count * 3);
                        for (size_t v = 0; v < vertex_count; v++) {
                            cgltf_accessor_read_float(accessor, v, &mesh->positions[v * 3], 3);
                        }
                        break;
                    }
                    case cgltf_attribute_type_normal: {
                        mesh->normals.resize(vertex_count * 3);
                        for (size_t v = 0; v < vertex_count; v++) {
                            cgltf_accessor_read_float(accessor, v, &mesh->normals[v * 3], 3);
                        }
                        break;
                    }
                    case cgltf_attribute_type_texcoord: {
                        mesh->uvs.resize(vertex_count * 2);
                        for (size_t v = 0; v < vertex_count; v++) {
                            cgltf_accessor_read_float(accessor, v, &mesh->uvs[v * 2], 2);
                        }
                        break;
                    }
                    case cgltf_attribute_type_weights: {
                        mesh->weights.resize(vertex_count * 4);
                        for (size_t v = 0; v < vertex_count; v++) {
                            cgltf_accessor_read_float(accessor, v, &mesh->weights[v * 4], 4);
                        }
                        break;
                    }
                    case cgltf_attribute_type_joints: {
                        mesh->jointIndices.resize(vertex_count * 4);
                        for (size_t v = 0; v < vertex_count; v++) {
                            cgltf_uint joints[4];
                            cgltf_accessor_read_uint(accessor, v, joints, 4);
                            for (int ji = 0; ji < 4; ji++) {
                                mesh->jointIndices[v * 4 + ji] = static_cast<uint16_t>(joints[ji]);
                            }
                        }
                        break;
                    }
                }
            }

            // Process indices
            if (primitive.indices) {
                size_t index_count = primitive.indices->count;
                mesh->indices.resize(index_count);
                for (size_t idx = 0; idx < index_count; idx++) {
                    mesh->indices[idx] = cgltf_accessor_read_index(primitive.indices, idx);
                }
            }

            // Create placeholder material
            mesh->material = std::make_shared<Material>();
            mesh->material->name = primitive.material && primitive.material->name ? 
                                 primitive.material->name : "material";
        }

        model->meshes.push_back(mesh);
    }

    // Store model
    _self->models[name] = std::move(model);

    cgltf_free(data);
    return true;
}

bool AnimationProvider::HasModel(const char* name) const {
    return _self->models.find(name) != _self->models.end();
}

const Model* AnimationProvider::GetModel(const char* name) const {
    auto it = _self->models.find(name);
    return it != _self->models.end() ? it->second.get() : nullptr;
}

std::vector<std::string> AnimationProvider::GetModelNames() const {
    std::vector<std::string> names;
    names.reserve(_self->models.size());
    for (const auto& pair : _self->models) {
        names.push_back(pair.first);
    }
    return names;
}

void AnimationProvider::UnloadModel(const char* name) {
    _self->models.erase(name);
}

void AnimationProvider::UnloadAllModels() {
    _self->models.clear();
}

bool AnimationProvider::LoadAnimationFromBVH(const char* name, const char* filename, const char* skeletonName) {
    std::unique_ptr<BVHData> bvh(lab::ParseBVH(filename));
    if (!bvh || !bvh->root || bvh->frames.empty()) {
        return false;
    }

    // Get skeleton
    auto skeleton = GetSkeleton(skeletonName);
    if (!skeleton) {
        return false;
    }

    // Create raw animation
    ozz::animation::offline::RawAnimation raw_animation;
    raw_animation.duration = bvh->frameTime * bvh->frames.size();
    raw_animation.tracks.resize(skeleton->num_joints());

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
    auto animation = builder(raw_animation);
    if (!animation) {
        return false;
    }

    // Store animation
    _self->animations[name] = std::move(animation);

    return true;
}



bool AnimationProvider::HasAnimation(const char* name) const {
    return _self->animations.find(name) != _self->animations.end();
}

const ozz::animation::Animation* AnimationProvider::GetAnimation(const char* name) const {
    auto it = _self->animations.find(name);
    return it != _self->animations.end() ? it->second.get() : nullptr;
}

std::vector<std::string> AnimationProvider::GetAnimationNames() const {
    std::vector<std::string> names;
    names.reserve(_self->animations.size());
    for (const auto& pair : _self->animations) {
        names.push_back(pair.first);
    }
    return names;
}

AnimationProvider::Instance* AnimationProvider::CreateInstance(const char* name, const char* skeletonName, const char* animationName) {
    // Verify resources exist
    auto skeleton = GetSkeleton(skeletonName);
    auto animation = GetAnimation(animationName);
    if (!skeleton || !animation) {
        return nullptr;
    }

    // Create instance
    auto instance = std::make_unique<Instance>();
    instance->skeletonName = skeletonName;
    instance->animationName = animationName;

    // Initialize buffers
    const int num_joints = skeleton->num_joints();
    const int num_soa_joints = (num_joints + 3) / 4;
    instance->locals.resize(num_soa_joints);
    instance->models.resize(num_joints);
    instance->jointWeights.resize(num_joints, 1.f);

    // Store and return
    auto ptr = instance.get();
    _self->instances[name] = std::move(instance);
    return ptr;
}

void AnimationProvider::DestroyInstance(const char* name) {
    _self->instances.erase(name);
}

AnimationProvider::Instance* AnimationProvider::GetInstance(const char* name) {
    auto it = _self->instances.find(name);
    return it != _self->instances.end() ? it->second.get() : nullptr;
}

const AnimationProvider::Instance* AnimationProvider::GetInstance(const char* name) const {
    auto it = _self->instances.find(name);
    return it != _self->instances.end() ? it->second.get() : nullptr;
}

std::vector<std::string> AnimationProvider::GetInstanceNames() const {
    std::vector<std::string> names;
    names.reserve(_self->instances.size());
    for (const auto& pair : _self->instances) {
        names.push_back(pair.first);
    }
    return names;
}

float AnimationProvider::GetCurrentTime(const char* instanceName) const {
    auto instance = GetInstance(instanceName);
    return instance ? instance->currentTime : 0.0f;
}

void AnimationProvider::SetCurrentTime(const char* instanceName, float time) {
    auto instance = GetInstance(instanceName);
    if (!instance) return;

    auto animation = GetAnimation(instance->animationName.c_str());
    if (animation) {
        instance->currentTime = std::fmod(time, animation->duration());
    }
}

float AnimationProvider::GetDuration(const char* instanceName) const {
    auto instance = GetInstance(instanceName);
    if (!instance) return 0.0f;

    auto animation = GetAnimation(instance->animationName.c_str());
    return animation ? animation->duration() : 0.0f;
}

bool AnimationProvider::IsPlaying(const char* instanceName) const {
    auto instance = GetInstance(instanceName);
    return instance ? instance->playing : false;
}

void AnimationProvider::SetPlaying(const char* instanceName, bool playing) {
    auto instance = GetInstance(instanceName);
    if (instance) {
        instance->playing = playing;
    }
}

void AnimationProvider::Update(float deltaTime) {
    // Update all playing instances
    for (auto& pair : _self->instances) {
        auto& instance = pair.second;
        if (!instance->playing) continue;

        auto skeleton = GetSkeleton(instance->skeletonName.c_str());
        auto animation = GetAnimation(instance->animationName.c_str());
        if (!animation || !skeleton) continue;

        // Update animation time
        instance->currentTime = std::fmod(instance->currentTime + deltaTime, animation->duration());

        // Setup sampling job
        ozz::animation::SamplingJob sampling_job;
        sampling_job.animation = animation;
        sampling_job.ratio = instance->currentTime / animation->duration();
        sampling_job.output = ozz::make_span(instance->locals);

        // Sample animation
        if (!sampling_job.Run()) continue;

        // Convert local transforms to model-space matrices
        ozz::animation::LocalToModelJob ltm_job;
        ltm_job.skeleton = skeleton;
        ltm_job.input = ozz::make_span(instance->locals);
        ltm_job.output = ozz::make_span(instance->models);

        ltm_job.Run();
    }
}

const ozz::math::Float4x4* AnimationProvider::GetJointTransforms(const char* instanceName) const {
    auto instance = GetInstance(instanceName);
    return instance ? instance->models.data() : nullptr;
}

int AnimationProvider::GetJointCount(const char* instanceName) const {
    auto instance = GetInstance(instanceName);
    if (!instance) return 0;

    auto skeleton = GetSkeleton(instance->skeletonName.c_str());
    return skeleton ? skeleton->num_joints() : 0;
}

} // lab
