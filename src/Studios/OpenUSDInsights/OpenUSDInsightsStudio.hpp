//
//  Studios/OpenUSDInsights
//
//  Created by Nick Porcino on 15/12/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef Studios_OpenUSDInsights_hpp
#define Studios_OpenUSDInsights_hpp

#include "Lab/StudioCore.hpp"

namespace lab {
class OpenUSDInsightsStudio : public Studio
{
    struct data;
    data* _self;
public:
    explicit OpenUSDInsightsStudio();
    virtual ~OpenUSDInsightsStudio() override;
    virtual const std::vector<ActivityConfig>& StudioConfiguration() const override;
    virtual bool MustDeactivateUnrelatedActivitiesOnActivation() const override { return true; }

    virtual const std::string Name() const override { return sname(); }
    static constexpr std::string sname() { return "OpenUSD Insights"; }
};
}

#endif // Studios_OpenUSDInsights_hpp
