
#include "SpriteProvider.hpp"

#define CUTE_ASEPRITE_IMPLEMENTATION
#include "cute_ase_sprite.h"

#include <map>
#include <string>

namespace lab {


struct SpriteProvider::data {
    std::map<std::string, ase_t*> sprites;
    void cache_sprite(const char* filename, const char* id) {
        ase_t* sprites = cute_aseprite_load_from_file(filename, NULL);
        if (sprites) {
            this->sprites[id] = sprites;
        }
    }

    int frame_count(const char* id) {
        auto it = sprites.find(id);
        if (it != sprites.end()) {
            return it->second->frame_count;
        }
        return 0;
    }

    Frame frame(const char* id, int frame) {
        Frame f;
        f.w = 0;
        f.h = 0;
        f.durationMs = 0;
        f.pixels = nullptr;
        auto it = sprites.find(id);
        if (it != sprites.end()) {
            ase_t* sprite = it->second;

            // clamp frame to valid range
            if (frame < 0) frame = 0;
            if (frame >= sprite->frame_count) frame = sprite->frame_count - 1;
            if (frame >= 0) {
                ase_frame_t* fr = &sprite->frames[frame];
                f.w = sprite->w;
                f.h = sprite->h;
                f.durationMs = fr->duration_milliseconds;
                f.pixels = (uint8_t*)malloc(f.w * f.h * 4);
                memcpy(f.pixels, fr->pixels, f.w * f.h * 4);
            }
        }
        return f;
    }

};

SpriteProvider::SpriteProvider() : Provider(SpriteProvider::sname())
{
    _self = new SpriteProvider::data();
}

SpriteProvider::~SpriteProvider()
{
    delete _self;
}

SpriteProvider* SpriteProvider::instance()
{
    static SpriteProvider* _instance = new SpriteProvider();
    return _instance;
}

int SpriteProvider::frame_count(const char* id)
{
    return _self->frame_count(id);
}

SpriteProvider::Frame SpriteProvider::frame(const char* id, int frame)
{
    return _self->frame(id, frame);
}

void SpriteProvider::cache_sprite(const char* filename, const char* id)
{
    _self->cache_sprite(filename, id);
}

} // namespace lab
