
#include "MidiProvider.hpp"

#include "LabMidi/LabMidi.h"
#include <string>
#include <strstream>
#include <sstream>

namespace lab {

class MidiProvider::data
{
public:
    data()
    : midiPorts(new Lab::MidiPorts())
    {
    }

    ~data()
    {
        delete midiPorts;
    }

    std::string listPorts()
    {
        std::stringstream s;
        midiPorts->refreshPortList();
        int c = midiPorts->inPorts();
        if (c == 0)
            s << "No MIDI input ports found\n";
        else {
            s << "MIDI input ports:\n";
            for (int i = 0; i < c; ++i)
                s << "   " << i << ": " << midiPorts->inPort(i) << "\n";
        }

        c = midiPorts->outPorts();
        if (c == 0)
            s << "No MIDI output ports found\n";
        else {
            s << "MIDI output ports:\n";
            for (int i = 0; i < c; ++i)
                s << "   " << i << ": " << midiPorts->outPort(i) << "\n";
            s << std::endl;
        }
        report = s.str();
        return report;
    }

    Lab::MidiPorts* midiPorts;
    double startTime;
    std::string report;
};


MidiProvider* MidiProvider::_instance = nullptr;

MidiProvider::MidiProvider() : Provider(MidiProvider::sname()) {
    _self = new data();
    _instance = this;
    _self->listPorts();
    /// @TODO listPorts should be called whenever we might expect the list
    /// to have changed.
    provider.Documentation = [](void* instance) -> const char* {
        MidiProvider* mp = (MidiProvider*) instance;
        return mp->_self->report.c_str();
    };
}

MidiProvider::~MidiProvider() {
    delete _self;
}

MidiProvider* MidiProvider::instance() {
    return _instance;
}

} // lab
