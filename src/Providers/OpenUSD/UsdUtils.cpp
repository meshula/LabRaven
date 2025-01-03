#include "UsdUtils.hpp"
#include <pxr/usd/usdGeom/bboxCache.h>

using namespace PXR_NS;

GfMatrix4d GetTransformMatrix(UsdGeomXformable geom, UsdTimeCode timeCode)
{
    GfMatrix4d transform(1);
    transform = geom.ComputeLocalToWorldTransform(timeCode);
    return transform;
}

void SetTransformMatrix(UsdGeomXformable geom, GfMatrix4d transform, UsdTimeCode tc) {
    GfMatrix4d parentTransform =
        geom.ComputeParentToWorldTransform(tc);
    GfMatrix4d worldOffset = parentTransform.GetInverse() * transform;
    GfMatrix4d localOffset =
        parentTransform * worldOffset * parentTransform.GetInverse();
    
    /// @TODO why does creating a cube give me a transform Op of inverse?
    auto op = geom.GetTransformOp();
    if (op.GetOpType() != UsdGeomXformOp::TypeTransform) {
        geom.ClearXformOpOrder();
        geom.AddTransformOp().Set(localOffset);
    }
    else {
        op.Set(localOffset, tc);
    }
}

bool IsParentOf(UsdPrim prim, UsdPrim childPrim)
{
    if (!prim.IsValid() || !childPrim.IsValid()) return false;

    UsdPrim curParent = childPrim.GetParent();
    while (curParent.IsValid()) {
        if (curParent == prim) return true;
        curParent = curParent.GetParent();
    }
    return false;
}

UsdPrim GetInstanceableParent(UsdPrim prim)
{
    if (!prim.IsValid()) return UsdPrim();

    UsdPrim curParent = prim.GetParent();
    UsdPrim lastInstanceablePrim = UsdPrim();
    while (curParent.IsValid()) {
        if (curParent.IsInstanceable()) lastInstanceablePrim = curParent;
        curParent = curParent.GetParent();
    }
    return lastInstanceablePrim;
}

bool AreNearlyEquals(GfMatrix4f mat1, GfMatrix4f mat2,
                     float precision)
{
    GfMatrix4f subMat = mat1 - mat2;
    for (size_t i = 0; i < 16; i++)
        if (GfAbs(subMat.data()[i]) > precision) return false;
    return true;
}

int GetSize(UsdPrimSiblingRange range)
{
    return (int) std::distance(range.begin(), range.end());
}

pxr::GfBBox3d ComputeWorldBounds(UsdStageRefPtr stage, UsdTimeCode timeCode, pxr::SdfPathVector& prims) {
    UsdGeomBBoxCache bboxcache(timeCode, UsdGeomImageable::GetOrderedPurposeTokens());
    GfBBox3d bbox;
    for (const auto &primPath : prims) {
        auto prim = stage->GetPrimAtPath(primPath);
        bbox = GfBBox3d::Combine(bboxcache.ComputeWorldBound(prim), bbox);
    }
    return bbox;
}
