//
//  Studios/Texture
//
//  Created by Nick Porcino on 3/2/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef Studios_Texture_hpp
#define Studios_Texture_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
class TextureStudio : public Studio
{
    struct data;
    data* _self;
public:
    explicit TextureStudio();
    virtual ~TextureStudio() override;
    virtual const std::vector<std::string>& StudioConfiguration() const override;
    virtual bool MustDeactivateUnrelatedActivitiesOnActivation() const override { return true; }

    virtual const std::string Name() const override { return sname(); }
    static constexpr std::string sname() { return "Texture Studio"; }
};
}

#endif // Studios_Texture_hpp
