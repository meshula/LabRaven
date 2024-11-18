
#ifndef PREFERENCES_ACTIVITY_HPP
#define PREFERENCES_ACTIVITY_HPP

#include "Lab/StudioCore.hpp"

namespace lab {

class PreferencesActivity : public Activity
{
    struct data;
    data* _self;
    void RunUI(const LabViewInteraction&);

public:
    explicit PreferencesActivity();
    virtual ~PreferencesActivity();

    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Preferences"; }
};

} // lab

#endif // PREFERENCES_ACTIVITY_HPP
