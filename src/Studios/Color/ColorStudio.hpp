//
//  Studios/Color
//
//  Created by Nick Porcino on 15/12/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef Studios_Color_hpp
#define Studios_Color_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
class ColorStudio : public Studio
{
    struct data;
    data* _self;
public:
    explicit ColorStudio();
    virtual ~ColorStudio() override;
    virtual const std::vector<std::string>& StudioConfiguration() const override;
    virtual bool MustDeactivateUnrelatedActivitiesOnActivation() const override { return true; }

    virtual const std::string Name() const override { return sname(); }
    static constexpr std::string sname() { return "Color Studio"; }
};
}

#endif // Studios_Color_hpp
