//
//  LabBVH.hpp
//  raven
//
//  Created by Nick Porcino on 2/7/25.
//
#ifndef LABBVH_HPP
#define LABBVH_HPP

#include <string>
#include <vector>

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

BVHData* ParseBVH(const char* filename);

} // lab

#endif

