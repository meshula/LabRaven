
#ifndef ProvidersActivity_hpp
#define ProvidersActivity_hpp

#include <string>
#include <vector>
#include <memory>

#include "Lab/StudioCore.hpp"

namespace lab {

class ProvidersActivity : public Activity
{
    struct data;
    data* _self;
    void RunUI(const LabViewInteraction&);

public:
    explicit ProvidersActivity();
    virtual ~ProvidersActivity();

    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Providers"; }
};

} // lab

#endif /* ProvidersActivity_hpp */

