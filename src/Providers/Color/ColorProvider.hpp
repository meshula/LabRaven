
#ifndef Providers_Color_ColorProvider_hpp
#define Providers_Color_ColorProvider_hpp

#include "Lab/StudioCore.hpp"

namespace lab {

class ColorProvider : public Provider {
    struct data;
    data* _self;
    static ColorProvider* _instance;
public:
    ColorProvider();
    ~ColorProvider();

    static ColorProvider* instance();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Color"; }

    const char* RenderingColorSpace() const;
    void SetRenderingColorSpace(const char* colorSpace);
};

} // lab
#endif // Providers_Color_ColorProvider_hpp
