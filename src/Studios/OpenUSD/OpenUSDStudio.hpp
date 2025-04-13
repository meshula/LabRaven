//
//  Studios/OpenUSD
//
//  Created by Nick Porcino on 15/12/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef Studios_OpenUSD_hpp
#define Studios_OpenUSD_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
class OpenUSDStudio : public Studio
{
    struct data;
    data* _self;
public:
    explicit OpenUSDStudio();
    virtual ~OpenUSDStudio() override;
    virtual const std::vector<ActivityConfig>& StudioConfiguration() const override;
    virtual bool MustDeactivateUnrelatedActivitiesOnActivation() const override { return true; }

    virtual const std::string Name() const override { return sname(); }
    static constexpr std::string sname() { return "OpenUSD Studio"; }
};
}

#endif // Studios_OpenUSD_hpp
