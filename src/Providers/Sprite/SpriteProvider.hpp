#ifndef SpriteProvider_hpp
#define SpriteProvider_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
class SpriteProvider : public Provider
{
    struct data;
    data* _self;
public:
    explicit SpriteProvider();
    virtual ~SpriteProvider() override;

    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Sprite"; }

    static SpriteProvider* instance();

    //--------------------------------------------------------------------------------

    struct Frame {
        int w, h;
        int durationMs;
        uint8_t* pixels;
    };

    int frame_count(const char* id);
    Frame frame(const char* id, int frame);
    void cache_sprite(const char* filename, const char* id);
};
}

#endif /* SpriteProvider_hpp */
