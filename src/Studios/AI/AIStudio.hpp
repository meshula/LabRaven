//
//  Studios/AI/AIStudio.hpp
//  LabRaven
//
//  AI Studio - Activates AI and LLM-related activities
//

#ifndef Studios_AI_hpp
#define Studios_AI_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
class AIStudio : public Studio
{
    struct data;
    data* _self;
public:
    explicit AIStudio();
    virtual ~AIStudio() override;
    virtual const std::vector<ActivityConfig>& StudioConfiguration() const override;
    virtual bool MustDeactivateUnrelatedActivitiesOnActivation() const override { return true; }

    virtual const std::string Name() const override { return sname(); }
    static constexpr std::string sname() { return "AI Studio"; }
};
}

#endif // Studios_AI_hpp
