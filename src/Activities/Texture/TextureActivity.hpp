//
//  TextureActivity.hpp
//  LabExcelsior
//
//  Created by Nick Porcino on 12/7/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#ifndef TextureActivity_hpp
#define TextureActivity_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
class TextureActivity : public Activity
{
    struct data;
    data* _self;
    void RunTextureCachePanel();
    void RunTextureInspectorPanel();
    
    // activities
    void RunUI(const LabViewInteraction&);
    void Menu();
    void Update();

public:
    TextureActivity();
    virtual ~TextureActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr std::string sname() { return "Texture"; }
};

} //lab

#endif /* TextureActivity_hpp */
