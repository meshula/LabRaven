
#ifndef Providers_Color_ColorProvider_hpp
#define Providers_Color_ColorProvider_hpp

namespace lab {

class ColorProvider {
    struct data;
    data* _self;
    static ColorProvider* _instance;
public:
    ColorProvider();
    ~ColorProvider();

    static ColorProvider* instance();

    const char* RenderingColorSpace() const;
    void SetRenderingColorSpace(const char* colorSpace);
};

} // lab
#endif // Providers_Color_ColorProvider_hpp
