//
//  ColorActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 12/6/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#ifndef ColorActivity_hpp
#define ColorActivity_hpp

#include "Lab/StudioCore.hpp"

namespace lab {

class ColorActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void Render(const LabViewInteraction&);
    void RunUI(const LabViewInteraction&);

public:
    explicit ColorActivity();
    virtual ~ColorActivity();

    std::string GetRenderingColorSpace() const;
    void SetRenderingColorSpace(const std::string& colorSpace);
    
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Color"; }
};

} // lab

#endif /* ColorActivity_hpp */
