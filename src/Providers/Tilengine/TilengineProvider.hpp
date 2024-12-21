
#ifndef Providers_Tilengine_TilengineProvider_hpp
#define Providers_Tilengine_TilengineProvider_hpp

#include "Lab/StudioCore.hpp"

namespace lab {

class TilengineProvider : public Provider {
    struct data;
    data* _self;
    static TilengineProvider* _instance;
public:
    TilengineProvider();
    ~TilengineProvider();

    static TilengineProvider* instance();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Tilengine"; }
};

} // lab
#endif // Providers_Tilengine_TilengineProvider_hpp
