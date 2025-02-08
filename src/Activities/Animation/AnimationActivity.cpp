#include "AnimationActivity.hpp"
#include "Lab/App.h"
#include "Providers/Animation/AnimationProvider.hpp"
#include <ozz/base/maths/simd_math.h>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "implot3d.h"

namespace lab {

struct AnimationActivity::data {
    bool showAddSkeletonModal = false;
    bool showAddAnimationModal = false;
    char skeletonFilePath[256] = "";
    char animationFilePath[256] = "";
    char skeletonName[256] = "";
    char animationName[256] = "";
    std::string selectedSkeleton;
    std::string selectedAnimation;
};

AnimationActivity::AnimationActivity() 
: Activity(AnimationActivity::sname())
, _self(new data) {
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<AnimationActivity*>(instance)->RunUI(*vi);
    };
}

AnimationActivity::~AnimationActivity() {
    delete _self;
}

void AnimationActivity::_activate() {
}

void AnimationActivity::_deactivate() {
}

void AnimationActivity::RunUI(const LabViewInteraction&) {
    ImGui::Begin("Animation##mM");

    // Left panel with lists
    ImGui::BeginChild("left_panel", ImVec2(200, 0), true);
    
    // Skeletons list
    ImGui::Text("Skeletons");
    ImGui::BeginChild("skeletons", ImVec2(0, 200), true);
    auto skeletonNames = AnimationProvider::instance()->GetSkeletonNames();
    for (const auto& name : skeletonNames) {
        ImGui::PushID(name.c_str());
        if (ImGui::Selectable(name.c_str(), _self->selectedSkeleton == name)) {
            _self->selectedSkeleton = name;
            _self->selectedAnimation = ""; // Clear animation selection when skeleton changes
        }
        ImGui::SameLine();
        if (ImGui::Button("X")) {
            AnimationProvider::instance()->UnloadSkeleton(name.c_str());
            if (_self->selectedSkeleton == name) {
                _self->selectedSkeleton = "";
            }
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
    if (ImGui::Button("+ Skeleton")) {
        _self->showAddSkeletonModal = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Animations list
    ImGui::Text("Animations");
    ImGui::BeginChild("animations", ImVec2(0, 200), true);
    auto animationNames = AnimationProvider::instance()->GetAnimationNames();
    for (const auto& name : animationNames) {
        ImGui::PushID(name.c_str());
        if (ImGui::Selectable(name.c_str(), _self->selectedAnimation == name)) {
            _self->selectedAnimation = name;
        }
        ImGui::SameLine();
        if (ImGui::Button("X")) {
            AnimationProvider::instance()->UnloadAnimation(name.c_str());
            if (_self->selectedAnimation == name) {
                _self->selectedAnimation = "";
            }
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
    if (ImGui::Button("+ Animation")) {
        _self->showAddAnimationModal = true;
    }

    ImGui::EndChild();

    ImGui::SameLine();

    // Center panel with 3D plot and controls
    ImGui::BeginChild("center_panel", ImVec2(-200, 0));
    
    // 3D Plot
    if (ImPlot3D::BeginPlot("##3D", ImVec2(-1, -50))) {
        ImPlot3D::SetupAxes("x", "z", "y");
        if (!_self->selectedSkeleton.empty()) {
            auto skeleton = AnimationProvider::instance()->GetSkeleton(_self->selectedSkeleton.c_str());
            if (skeleton) {
                if (_self->selectedAnimation.empty()) {
                    do {
                        const int num_joints = skeleton->num_joints();
                        if (!num_joints) {
                            break;
                        }

                        // Reallocate matrix array if necessary.
                        static std::vector<ozz::math::Float4x4> prealloc_models_;
                        static std::vector<float> xs;
                        static std::vector<float> ys;
                        static std::vector<float> zs;

                        prealloc_models_.resize(num_joints);
                        xs.resize(num_joints);
                        ys.resize(num_joints);
                        zs.resize(num_joints);

                        // Compute model space rest pose.
                        ozz::animation::LocalToModelJob job;
                        job.input = skeleton->joint_rest_poses();
                        job.output = ozz::make_span(prealloc_models_);
                        job.skeleton = skeleton;
                        if (!job.Run()) {
                            break;
                        }

                        for (size_t i = 0; i < num_joints; ++i) {
                            // Implot3d is z up, so swap zy
                            auto& transform = prealloc_models_[i];
                            ozz::math::SimdFloat4 vp = ozz::math::TransformPoint(transform, ozz::math::simd_float4::zero());
                            xs[i] = vp.x;
                            ys[i] = vp.z;
                            zs[i] = vp.y;
                        }

                        ImPlot3D::PlotScatter("##joints-plot",
                                              xs.data(), ys.data(), zs.data(),
                                              num_joints);
                    } while (false);
                }
                else {
                    // Get joint transforms from the instance if we have one
                    const ozz::math::Float4x4* transforms = nullptr;
                    if (!_self->selectedAnimation.empty()) {
                        auto instance = AnimationProvider::instance()->GetInstance("current");
                        if (instance) {
                            transforms = AnimationProvider::instance()->GetJointTransforms("current");
                        }
                    }

                    // Draw each joint and its parent connection
                    int numJoints = skeleton->num_joints();
                    const ozz::math::Float4x4* matrices = transforms;

                    // If no transforms (no animation), use identity matrices
                    std::vector<ozz::math::Float4x4> identityMatrices;
                    if (!matrices) {
                        identityMatrices.resize(numJoints, ozz::math::Float4x4::identity());
                        matrices = identityMatrices.data();
                    }

                    // Collect bone positions
                    std::vector<float> xs, ys, zs;
                    int lineCount = 0;
                    for (int i = 0; i < numJoints; i++) {
                        int parentIdx = skeleton->joint_parents()[i];
                        if (parentIdx != -1) {
                            // Extract positions from matrices
                            const auto& childMat = matrices[i];
                            const auto& parentMat = matrices[parentIdx];

                            // Add parent point
                            xs.push_back(parentMat.cols[3].x);
                            ys.push_back(parentMat.cols[3].y);
                            zs.push_back(parentMat.cols[3].z);

                            // Add child point
                            xs.push_back(childMat.cols[3].x);
                            ys.push_back(childMat.cols[3].y);
                            zs.push_back(childMat.cols[3].z);

                            lineCount += 2;
                        }
                    }

                    // Draw all bones in one call
                    if (lineCount > 0) {
                        ImPlot3D::PlotLine("##bones",
                                           xs.data(), ys.data(), zs.data(),
                                           lineCount);
                    }

                    // Collect joint positions
                    xs.clear();
                    ys.clear();
                    zs.clear();
                    for (int i = 0; i < numJoints; i++) {
                        const auto& mat = matrices[i];
                        xs.push_back(mat.cols[3].x);
                        ys.push_back(mat.cols[3].y);
                        zs.push_back(mat.cols[3].z);
                    }

                    // Draw all joints in one call
                    ImPlot3D::PlotScatter("##joints",
                                          xs.data(), ys.data(), zs.data(),
                                          numJoints);
                }
            }
        }
        ImPlot3D::EndPlot();
    }

    // Animation controls
    if (!_self->selectedAnimation.empty()) {
        auto instance = AnimationProvider::instance()->GetInstance("current");
        if (!instance) {
            instance = AnimationProvider::instance()->CreateInstance("current", 
                _self->selectedSkeleton.c_str(), _self->selectedAnimation.c_str());
        }

        if (instance) {
            bool playing = AnimationProvider::instance()->IsPlaying("current");
            if (ImGui::Button(playing ? "Stop" : "Play")) {
                AnimationProvider::instance()->SetPlaying("current", !playing);
            }
            ImGui::SameLine();
            float duration = AnimationProvider::instance()->GetDuration("current");
            float currentTime = AnimationProvider::instance()->GetCurrentTime("current");
            ImGui::Text("Time: %.2f / %.2f", currentTime, duration);
        }
    }

    ImGui::EndChild();

    ImGui::SameLine();

    // Right panel with joint hierarchy
    ImGui::BeginChild("right_panel", ImVec2(0, 0), true);
    ImGui::Text("Joint Hierarchy");
    if (!_self->selectedSkeleton.empty()) {
        auto skeleton = AnimationProvider::instance()->GetSkeleton(_self->selectedSkeleton.c_str());
        if (skeleton) {
            const ozz::math::Float4x4* transforms = nullptr;
            if (!_self->selectedAnimation.empty()) {
                auto instance = AnimationProvider::instance()->GetInstance("current");
                if (instance) {
                    transforms = AnimationProvider::instance()->GetJointTransforms("current");
                }
            }

            // If no transforms, use identity matrices
            std::vector<ozz::math::Float4x4> identityMatrices;
            if (!transforms) {
                int numJoints = skeleton->num_joints();
                identityMatrices.resize(numJoints, ozz::math::Float4x4::identity());
                transforms = identityMatrices.data();
            }

            // Display joint hierarchy
            std::function<void(int, int)> displayJoint = [&](int jointIndex, int depth) {
                // Indent based on depth
                for (int i = 0; i < depth; i++) {
                    ImGui::Indent();
                }

                // Get joint name
                const char* name = skeleton->joint_names()[jointIndex];
                const auto& transform = transforms[jointIndex];
                ImGui::Text("%s (index: %d, pos: %.2f, %.2f, %.2f)", 
                    name, jointIndex,
                    transform.cols[3].x,
                    transform.cols[3].y,
                    transform.cols[3].z
                );

                // Process children
                int numJoints = skeleton->num_joints();
                const auto& parents = skeleton->joint_parents();
                for (int i = 0; i < numJoints; i++) {
                    if (parents[i] == jointIndex) {
                        displayJoint(i, depth + 1);
                    }
                }

                // Unindent
                for (int i = 0; i < depth; i++) {
                    ImGui::Unindent();
                }
            };

            // Start with root joints (parent index == -1)
            const auto& parents = skeleton->joint_parents();
            int numJoints = skeleton->num_joints();
            for (int i = 0; i < numJoints; i++) {
                if (parents[i] == -1) {
                    displayJoint(i, 0);
                }
            }
        }
    }
    ImGui::EndChild();

    // Add Skeleton Modal
    if (_self->showAddSkeletonModal) {
        ImGui::OpenPopup("Add Skeleton");
    }
    if (ImGui::BeginPopupModal("Add Skeleton", &_self->showAddSkeletonModal)) {
        ImGui::InputText("Name", _self->skeletonName, sizeof(_self->skeletonName));
        ImGui::InputText("File Path", _self->skeletonFilePath, sizeof(_self->skeletonFilePath));
        if (ImGui::Button("Add")) {
            if (strlen(_self->skeletonName) > 0 && strlen(_self->skeletonFilePath) > 0) {
                AnimationProvider::instance()->LoadSkeletonFromBVH(_self->skeletonName, _self->skeletonFilePath);
                _self->showAddSkeletonModal = false;
                memset(_self->skeletonName, 0, sizeof(_self->skeletonName));
                memset(_self->skeletonFilePath, 0, sizeof(_self->skeletonFilePath));
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            _self->showAddSkeletonModal = false;
        }
        ImGui::EndPopup();
    }

    // Add Animation Modal
    if (_self->showAddAnimationModal) {
        ImGui::OpenPopup("Add Animation");
    }
    if (ImGui::BeginPopupModal("Add Animation", &_self->showAddAnimationModal)) {
        ImGui::InputText("Name", _self->animationName, sizeof(_self->animationName));
        ImGui::InputText("File Path", _self->animationFilePath, sizeof(_self->animationFilePath));
        if (ImGui::Button("Add")) {
            if (strlen(_self->animationName) > 0 && strlen(_self->animationFilePath) > 0 && !_self->selectedSkeleton.empty()) {
                AnimationProvider::instance()->LoadAnimationFromBVH(_self->animationName, _self->animationFilePath, _self->selectedSkeleton.c_str());
                _self->showAddAnimationModal = false;
                memset(_self->animationName, 0, sizeof(_self->animationName));
                memset(_self->animationFilePath, 0, sizeof(_self->animationFilePath));
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            _self->showAddAnimationModal = false;
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

} // namespace lab
