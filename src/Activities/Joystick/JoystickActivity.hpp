#ifndef ACTIVITIES_JOYSTICKACTIVITY_HPP
#define ACTIVITIES_JOYSTICKACTIVITY_HPP

#include "Lab/StudioCore.hpp"

namespace lab {

class JoystickActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);
    virtual void _activate() override;
    virtual void _deactivate() override;

public:
    explicit JoystickActivity();
    virtual ~JoystickActivity();
    
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Joystick"; }
};

} // namespace lab

#endif // ACTIVITIES_JOYSTICKACTIVITY_HPP
