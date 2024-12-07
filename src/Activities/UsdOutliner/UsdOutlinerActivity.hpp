//
//  UsdOutlinerActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 12/16/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#ifndef UsdOutlinerActivity_hpp
#define UsdOutlinerActivity_hpp

#include "Lab/StudioCore.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include <pxr/usd/usd/stage.h>

namespace lab {
class UsdOutlinerActivity : public Activity
{
    struct data;
    data* _self;
    ImRect DrawPrimHierarchy(pxr::UsdPrim prim);
    bool DrawHierarchyNode(pxr::UsdPrim prim, ImRect& curItemRect);
    void DrawChildrendHierarchyDecoration(ImRect parentRect,
                                          std::vector<ImRect> childrenRects);
    ImGuiTreeNodeFlags ComputeDisplayFlags(pxr::UsdPrim prim);
    bool IsParentOfModelSelection(pxr::UsdPrim prim) const;
    bool IsSelected(pxr::UsdPrim prim) const;

    virtual void _activate() override;
    
    // activities
    void RunUI(const LabViewInteraction&);

public:
    UsdOutlinerActivity();
    virtual ~UsdOutlinerActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Outliner"; }
};
}

#endif /* UsdOutlinerActivity_hpp */
