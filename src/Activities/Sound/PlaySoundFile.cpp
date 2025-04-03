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
};

SoundSampleBase::SoundSampleBase() {
    _self_base = new data();
}
SoundSampleBase::~SoundSampleBase() {
    delete _self_base;
}

std::shared_ptr<AudioBus> SoundSampleBase::MakeBusFromSampleFile(char const* const name, float sampleRate)
{
    return SoundProvider::instance()->MakeBusFromSampleFile(name, sampleRate);
}

struct PlaySoundFile::data {
    std::shared_ptr<SampledAudioNode> musicClipNode;
    std::shared_ptr<GainNode> gain;
    std::shared_ptr<PeakCompNode> peakComp;
    std::shared_ptr<AudioBus> musicClip;
    std::string file;
    bool graphMade = false;
    
    data() {
        file = "samples/stereo-music-clip.wav";
    }
    ~data() {
        auto& ac = *SoundProvider::instance()->Context();
        ac.disconnect(gain);
        ac.disconnect(peakComp);
        ac.disconnect(musicClipNode);
    }
    
    void LoadFile() {
        if (musicClip) { return; }

        auto& ac = *SoundProvider::instance()->Context();
        if (!musicClipNode) {
            musicClipNode = std::make_shared<SampledAudioNode>(ac);
        }

        musicClip = MakeBusFromSampleFile(file.c_str(), ac.sampleRate());
        if (musicClip) {
            ContextRenderLock r(&ac, "PlaySoundFile");
            musicClipNode->setBus(musicClip);
        }
    }
    
    void MakeGraph() {
        if (graphMade) { return; }
        graphMade = true;
        LoadFile();

        auto& ac = *SoundProvider::instance()->Context();
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();

        gain = std::make_shared<GainNode>(ac);
        gain->gain()->setValue(0.5f);
        
        // osc -> gain -> destination
        ac.connect(gain, musicClipNode, 0, 0);
        /// @dp ac.connect(ac.destinationNode(), _self->gain, 0, 0);

        ac.synchronizeConnections();

        printf("------\n");
        ac.debugTraverse(ac.destinationNode().get());
        printf("------\n");
    }
    
    void Play() {
        MakeGraph();
        musicClipNode->schedule(0.f);
    }
    
    void Stop() {
        if (musicClipNode) {
            musicClipNode->stop(0.f);
        }
    }
    
    bool IsPlaying() {
        MakeGraph();
        if (!musicClipNode) {
            return false;
        }
        return musicClipNode->isPlayingOrScheduled();
    }
};

PlaySoundFile::PlaySoundFile() {
    _self = new data();
}

PlaySoundFile::~PlaySoundFile() {
    printf("Destructing %s\n", name());
    delete _self;
}

void PlaySoundFile::RunUI() {
    auto ac = SoundProvider::instance()->Context();
    ImGui::BeginChild("###PSF-RUI");
    if (!ac) {
        ImGui::TextUnformatted("No context");
    }
    else {
        _self->LoadFile();
        ImGui::Text("Sound file: %s", _self->file.c_str());
        auto clip = _self->musicClip;
        float rate = clip? clip->sampleRate() : 0.f;
        if (rate <= 0) {
            rate = ac->sampleRate();
        }
        float len = clip? (float) clip->length() / rate : 0.f;
        ImGui::Text(" channels: %d length: %0.3gs", clip? clip->numberOfChannels() : 0, len);
        bool isPlaying = _self->IsPlaying();
        if (!isPlaying) {
            if (ImGui::Button("Play##PSF-RUI")) {
                _self->Play();
            }
        }
        else {
            if (ImGui::Button("Pause##PSF-RUI")) {
                _self->Stop();
            }
        }
    }
    ImGui::EndChild();
}
