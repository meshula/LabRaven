
#ifndef TilengineActivity_hpp
#define TilengineActivity_hpp

#include <string>
#include <vector>
#include <memory>

#include "Lab/StudioCore.hpp"

namespace lab {

class TilengineActivity : public Activity
{
    struct data;
    data* _self;
    void RunUI(const LabViewInteraction&);
    virtual void _activate();
    virtual void _deactivate();

public:
    explicit TilengineActivity();
    virtual ~TilengineActivity();

    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Tilengine"; }
};

} // lab

#endif /* TilengineActivity_hpp */

