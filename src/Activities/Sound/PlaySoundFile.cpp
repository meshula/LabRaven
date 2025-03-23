#include "PlaySoundFile.hpp"
#include "SoundActivity.hpp"
#include "Lab/App.h"
#include "Providers/Sound/SoundProvider.hpp"
#include "Lab/LabDirectories.h"

#include "LabSound/LabSound.h"
#include "LabSound/extended/Util.h"

#include <map>
#include <string>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

using namespace lab;

//-----------------------------------------------------------------------------

struct SoundSampleBase::data {
    std::map<std::string, std::shared_ptr<AudioBus>> sample_cache;
};

SoundSampleBase::SoundSampleBase() {
    _self_base = new data();
}
SoundSampleBase::~SoundSampleBase() {
    delete _self_base;
}

std::shared_ptr<AudioBus> SoundSampleBase::MakeBusFromSampleFile(char const* const name, float sampleRate)
{
    auto it = _self_base->sample_cache.find(name);
    if (it != _self_base->sample_cache.end())
        return it->second;

    std::string path_prefix = lab_application_resource_path(nullptr, nullptr);
    
    const std::string path = path_prefix + "/" + std::string(name);
    std::shared_ptr<AudioBus> bus = MakeBusFromFile(path, false, sampleRate);
    if (!bus)
        throw std::runtime_error("couldn't open " + path);

        _self_base->sample_cache[name] = bus;

    return bus;
}

struct PlaySoundFile::data {
    std::shared_ptr<SampledAudioNode> musicClipNode;
    std::shared_ptr<GainNode> gain;
    std::shared_ptr<PeakCompNode> peakComp;

    std::shared_ptr<AudioBus> musicClip;
};

PlaySoundFile::PlaySoundFile() {
    _self = new data();
}

PlaySoundFile::~PlaySoundFile() {
    printf("Destructing %s\n", name());
    delete _self;
}

void PlaySoundFile::play() {
    auto& ac = *SoundProvider::instance()->Context();
    ac.disconnect(ac.destinationNode());
    ac.synchronizeConnections();

    //auto musicClip = MakeBusFromSampleFile("samples/voice.ogg", argc, argv);
    auto musicClip = MakeBusFromSampleFile("samples/stereo-music-clip.wav", ac.sampleRate());
    if (!musicClip)
        return;

    _self->gain = std::make_shared<GainNode>(ac);
    _self->gain->gain()->setValue(0.5f);

    _self->musicClipNode = std::make_shared<SampledAudioNode>(ac);
    {
        ContextRenderLock r(&ac, "ex_simple");
        _self->musicClipNode->setBus(musicClip);
    }
    _self->musicClipNode->schedule(0.0);

    // osc -> gain -> destination
    ac.connect(_self->gain, _self->musicClipNode, 0, 0);
    ac.connect(ac.destinationNode(), _self->gain, 0, 0);

    ac.synchronizeConnections();

    printf("------\n");
    ac.debugTraverse(ac.destinationNode().get());
    printf("------\n");

    Wait(static_cast<uint32_t>(1000.f * musicClip->length() / musicClip->sampleRate()));
}
