#ifndef ACTIVITIES_APPACTIVITY_HPP
#define ACTIVITIES_APPACTIVITY_HPP

#include "Lab/StudioCore.hpp"

namespace lab {

class AppActivity : public Activity
{
    struct data;
    data* _self;

public:
    explicit AppActivity();
    virtual ~AppActivity();
    
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "App"; }
};

} // namespace lab

#endif // ACTIVITIES_APPACTIVITY_HPP
