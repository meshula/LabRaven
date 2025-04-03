
#ifndef Providers_SoundProvider_hpp
#define Providers_SoundProvider_hpp

#include "Lab/StudioCore.hpp"
#include "LabSound/LabSound.h"
#include <vector>

namespace lab {

class SoundProvider : public Provider {
    struct data;
    data* _self;
    static SoundProvider* _instance;
public:
    SoundProvider();
    ~SoundProvider();

    static SoundProvider* instance();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Sound"; }

    lab::AudioContext* Context() const;
    std::vector<lab::AudioDeviceInfo>& DeviceInfo() const;
    void CreateContext(const AudioStreamConfig& inStream, const AudioStreamConfig& outStream);

    void Power(const std::string& path, std::vector<float>& out);
    std::shared_ptr<AudioBus> MakeBusFromSampleFile(char const* const name, float sampleRate);
};

} // lab
#endif // Providers_SoundProvider_hpp
