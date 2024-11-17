
#ifndef PluginActivity_hpp
#define PluginActivity_hpp

#include <string>
#include <vector>
#include <memory>

#include "Lab/StudioCore.hpp"

namespace lab {

class PluginActivity  : public Activity
{
    struct data;
    data* _self;
    void RunUI(const LabViewInteraction&);

public:
    explicit PluginActivity();
    virtual ~PluginActivity();

    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Plugin"; }
};

} // lab

#endif /* PluginActivity_hpp */
