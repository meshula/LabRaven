//  ComponentActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 2/27/25.
//  Copyright © 2025 Nick Porcino. All rights reserved.
//

#ifndef ComponentActivity_hpp
#define ComponentActivity_hpp

#include "Lab/StudioCore.hpp"
#include <unordered_map>
#include <vector>

namespace lab {

class ComponentActivity : public Activity
{
    struct data;
    data* _self;
    
    // activities
    void RunUI(const LabViewInteraction&);
    void Menu();
    
    // Component analysis functions
    void PrintHierarchy(const PXR_NS::UsdPrim& prim, int depth = 0, bool prune = false);
    void AnalyzeHierarchy(const PXR_NS::UsdPrim& prim, 
                          std::unordered_map<std::string, int>& kindCounts,
                          std::unordered_map<std::string, std::vector<PXR_NS::UsdPrim>>& kindPrims);
    void PrintAnalysis(const std::unordered_map<std::string, int>& kindCounts);

public:
    explicit ComponentActivity();
    virtual ~ComponentActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Component"; }
};

} // lab

#endif /* ComponentActivity_hpp */
