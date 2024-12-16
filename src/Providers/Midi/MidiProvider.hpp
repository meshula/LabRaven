
#ifndef Providers_MidiProvider_hpp
#define Providers_MidiProvider_hpp

#include "Lab/StudioCore.hpp"

namespace lab {

class MidiProvider : public Provider {
    struct data;
    data* _self;
    static MidiProvider* _instance;
public:
    MidiProvider();
    ~MidiProvider();

    static MidiProvider* instance();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Midi"; }
};

} // lab
#endif // Providers_MidiProvider_hpp
