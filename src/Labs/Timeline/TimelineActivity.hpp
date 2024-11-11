//
//  TimelineActivity.hpp
//  LabRaven
//
//  Created by Domenico Porcino on 2024/11/09
//

#ifndef TimelineActivity_hpp
#define TimelineActivity_hpp

#include "Lab/Modes.hpp"

namespace lab {

class TimelineActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);

public:
    explicit TimelineActivity();
    virtual ~TimelineActivity();

    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Timeline"; }
};

} // lab

#endif /* TimelineActivity_hpp */
