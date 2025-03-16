
#include "SoundProvider.hpp"
#include "LabSound/LabSound.h"
#include "LabSound/extended/Util.h"
#include "LabSound/backends/AudioDevice_RtAudio.h"

namespace lab {


struct SoundProvider::data {
    AudioStreamConfig inputConfig;
    AudioStreamConfig outputConfig;
    std::vector<AudioDeviceInfo> audioDevices;
    AudioDeviceIndex default_output_device;
    AudioDeviceIndex default_input_device;
    bool hash_input = false;
    std::unique_ptr<lab::AudioContext> context;

    data() {
    }
    ~data() {
    }

    void Initialize(bool with_input = true) {
        if (!audioDevices.empty()) {
            // if devices have been found, audio's been intialized.
            return;
        }
        const std::vector<AudioDeviceInfo> audioDevices = lab::AudioDevice_RtAudio::MakeAudioDeviceList();
        AudioDeviceInfo defaultOutputInfo, defaultInputInfo;
        for (auto & info : audioDevices) {
            if (info.is_default_output)
                defaultOutputInfo = info;
            if (info.is_default_input)
                defaultInputInfo = info;
        }

        AudioStreamConfig outputConfig;
        if (defaultOutputInfo.index != -1) {
            outputConfig.device_index = defaultOutputInfo.index;
            outputConfig.desired_channels = std::min(uint32_t(2), defaultOutputInfo.num_output_channels);
            outputConfig.desired_samplerate = defaultOutputInfo.nominal_samplerate;
        }

        AudioStreamConfig inputConfig;
        if (with_input) {
            if (defaultInputInfo.index != -1) {
                inputConfig.device_index = defaultInputInfo.index;
                inputConfig.desired_channels = std::min(uint32_t(1), defaultInputInfo.num_input_channels);
                inputConfig.desired_samplerate = defaultInputInfo.nominal_samplerate;
            }
            else {
                throw std::invalid_argument("the default audio input device was requested but none were found");
            }
        }

        // RtAudio doesn't support mismatched input and output rates.
        // this may be a pecularity of RtAudio, but for now, force an RtAudio
        // compatible configuration
        if (defaultOutputInfo.nominal_samplerate != defaultInputInfo.nominal_samplerate) {
            float min_rate = std::min(defaultOutputInfo.nominal_samplerate, defaultInputInfo.nominal_samplerate);
            inputConfig.desired_samplerate = min_rate;
            outputConfig.desired_samplerate = min_rate;
            printf("Warning ~ input and output sample rates don't match, attempting to set minimum\n");
        }
    }
};

SoundProvider::SoundProvider() : Provider(SoundProvider::sname()) {
    _self = new SoundProvider::data();
    _instance = this;
    provider.Documentation = [](void* instance) -> const char* {
        return "Provides LabSound";
    };
}

SoundProvider::~SoundProvider() {
    delete _self;
}

SoundProvider* SoundProvider::_instance = nullptr;
SoundProvider* SoundProvider::instance() {
    if (!_instance)
        new SoundProvider();
    return _instance;
}

void SoundProvider::StartAudio() {
    _self->Initialize();
}

lab::AudioContext* SoundProvider::Context() const { 
    return _self->context.get(); 
}

std::vector<lab::AudioDeviceInfo>& SoundProvider::DeviceInfo() const {
    return _self->audioDevices;
}


} // lab
