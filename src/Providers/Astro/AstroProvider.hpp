
#ifndef Providers_AstroProvider_hpp
#define Providers_AstroProvider_hpp

#include "Lab/StudioCore.hpp"

namespace lab {

class AstroProvider : public Provider {
    struct data;
    data* _self;
    static AstroProvider* _instance;
public:
AstroProvider();
    ~AstroProvider();

    static AstroProvider* instance();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Astro"; }
};

} // lab
#endif // Providers_AstroProvider_hpp
