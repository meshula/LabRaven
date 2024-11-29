#ifndef UsdSessionLayer_hpp
#define UsdSessionLayer_hpp

#include <pxr/usd/sdf/layer.h>
#include <memory>
#include "ImGuiHydraEditor/src/models/model.h"

namespace lab {

class UsdSessionLayer {
    struct Self;
    std::unique_ptr<Self> self;
    
    public:
    /**
     * @brief Construct a new UsdSessionLayer object
     *
     * @param model the Model of the new UsdSessionLayer view
     */
    UsdSessionLayer(pxr::Model* model);
    ~UsdSessionLayer();

    PXR_NS::SdfLayerRefPtr GetSessionLayer();

};

} // lab
#endif // UsdSessionLayer_hpp
