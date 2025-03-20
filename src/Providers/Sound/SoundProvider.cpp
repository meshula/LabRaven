
#include "SoundProvider.hpp"
#include "LabSound/LabSound.h"
#include "LabSound/extended/Util.h"
#include "LabSound/backends/AudioDevice_RtAudio.h"
#include "LabSound/backends/AudioDevice_MockAudio.h"
#include "Lab/LabDirectories.h"

namespace lab {


struct SoundProvider::data {
    AudioStreamConfig inputConfig;
    AudioStreamConfig outputConfig;
    std::vector<AudioDeviceInfo> audioDevices;
    AudioDeviceIndex default_output_device;
    AudioDeviceIndex default_input_device;
    bool hash_input = false;
    std::shared_ptr<lab::AudioContext> context;
    std::shared_ptr<lab::AudioDevice> device;
    std::shared_ptr<lab::AudioDestinationNode> destinationNode;

    data() {
        audioDevices = lab::AudioDevice_RtAudio::MakeAudioDeviceList();
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
        static bool with_input = true;
        if (with_input) {
            if (defaultInputInfo.index != -1) {
                inputConfig.device_index = defaultInputInfo.index;
                inputConfig.desired_channels = std::min(uint32_t(1), defaultInputInfo.num_input_channels);
                inputConfig.desired_samplerate = defaultInputInfo.nominal_samplerate;
            }
            else {
                std::cerr << "the default audio input device was requested but none were found" << std::endl;
            }
        }

        // RtAudio doesn't support mismatched input and output rates.
        // this may be a pecularity of RtAudio, but for now, force an RtAudio
        // compatible configuration
        if (defaultOutputInfo.nominal_samplerate != defaultInputInfo.nominal_samplerate) {
            float min_rate = std::min(defaultOutputInfo.nominal_samplerate, defaultInputInfo.nominal_samplerate);
            inputConfig.desired_samplerate = min_rate;
            outputConfig.desired_samplerate = min_rate;
            std::cerr << "Warning ~ input and output sample rates don't match, attempting to set minimum" << std::endl;
        }

        std::cout << "SoundProvider: " << audioDevices.size() << " devices found." << std::endl;
    }

    ~data() {
        // release all references to the destination node
        destinationNode.reset();
        device->setDestinationNode(destinationNode);
        context->setDestinationNode(destinationNode);
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

lab::AudioContext* SoundProvider::Context() const {
    return _self->context.get(); 
}

void SoundProvider::CreateContext(const AudioStreamConfig& inStream,
                                  const AudioStreamConfig& outStream) {
    static bool useMock = false;
    if (useMock) {
        std::string outp = lab_temp_directory_path() + std::string("/mock.wav");
        auto mock =  new lab::AudioDevice_Mockaudio(inStream, outStream, 2 * 1024 * 1024, outp);
        _self->device = std::shared_ptr<lab::AudioDevice>(mock);
    }
    else {
        _self->device = std::make_shared<lab::AudioDevice_RtAudio>(inStream, outStream);
    }
    _self->context = std::make_shared<lab::AudioContext>(false, true);
    _self->destinationNode =
        std::make_shared<lab::AudioDestinationNode>(*_self->context.get(), _self->device);
    _self->device->setDestinationNode(_self->destinationNode);
    _self->context->setDestinationNode(_self->destinationNode);
}

std::vector<lab::AudioDeviceInfo>& SoundProvider::DeviceInfo() const {
    return _self->audioDevices;
}

} // lab
