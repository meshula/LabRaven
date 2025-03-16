#ifndef ACTIVITIES_SoundACTIVITY_HPP
#define ACTIVITIES_SoundACTIVITY_HPP

#include "Lab/StudioCore.hpp"

namespace lab {

class SoundActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);
    virtual void _activate() override;
    virtual void _deactivate() override;

public:
    explicit SoundActivity();
    virtual ~SoundActivity();
    
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Sound"; }
};

} // namespace lab

#endif // ACTIVITIES_SoundACTIVITY_HPP
