#ifndef ACTIVITIES_ANIMATIONACTIVITY_HPP
#define ACTIVITIES_ANIMATIONACTIVITY_HPP

#include "Lab/StudioCore.hpp"
#include "Activities/Animation/AnimationDialogModules.hpp"
#include <map>
#include <vector>
#include "implot3d.h"

namespace lab {

struct ImPlotCacheMesh {
    std::vector<ImPlot3DPoint> vertices;
    std::vector<unsigned int> indices;
};

class AnimationActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);
    virtual void _activate() override;
    virtual void _deactivate() override;

public:
    explicit AnimationActivity();
    virtual ~AnimationActivity();
    
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Animation"; }
};

} // namespace lab

#endif // ACTIVITIES_ANIMATIONACTIVITY_HPP
