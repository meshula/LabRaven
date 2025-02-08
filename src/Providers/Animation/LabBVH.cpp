//
//  LabBVH.cpp
//  raven
//
//  Created by Nick Porcino on 2/7/25.
//

#include "LabBVH.hpp"
#include <fstream>
#include <sstream>

namespace lab {

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

BVHData* ParseBVH(const char* filename) {
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

} // lab

