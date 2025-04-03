#ifndef ACTIVITIES_SOUND_PLAYSOUNDFILE_HPP
#define ACTIVITIES_SOUND_PLAYSOUNDFILE_HPP

#include <stdio.h>
#include <stdint.h>
#include <chrono>
#include <thread>

namespace lab { class AudioBus; }

class SoundSampleBase
{
    struct data;
    data* _self_base;

public:
    SoundSampleBase();
    virtual ~SoundSampleBase();

    virtual void Update() {}
    virtual void RunUI() {}
    virtual char const* const name() const = 0;

    static void Wait(uint32_t ms)
    {
        int32_t remainder = static_cast<int32_t>(ms);
        while (remainder > 200) {
            remainder -= 200;
            std::chrono::milliseconds t(200);
            std::this_thread::sleep_for(t);
        }
        if (remainder < 200 && remainder > 0) {
            std::chrono::milliseconds t(remainder);
            std::this_thread::sleep_for(t);
        }
    }

    static std::shared_ptr<lab::AudioBus> MakeBusFromSampleFile(char const* const name, float sampleRate);
};

class PlaySoundFile : public SoundSampleBase {
    struct data;
    data* _self;
public:    
    virtual char const* const name() const override { return "Play Sound File"; }

    explicit PlaySoundFile();
    virtual ~PlaySoundFile();

    virtual void RunUI() override final;
};

#endif // ACTIVITIES_SOUND_PLAYSOUNDFILE_HPP
