#ifndef ACTIVITIES_SPRITEACTIVITY_HPP
#define ACTIVITIES_SPRITEACTIVITY_HPP

#include "Lab/StudioCore.hpp"

namespace lab {

class SpriteActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);

public:
    explicit SpriteActivity();
    virtual ~SpriteActivity();
    
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Sprite"; }
};

} // namespace lab

#endif // ACTIVITIES_SPRITEACTIVITY_HPP
