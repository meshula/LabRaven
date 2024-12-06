#ifndef Providers_Selection_hpp
#define Providers_Selection_hpp

#include "Lab/StudioCore.hpp"

#ifndef HAVE_NO_USD
#include <pxr/base/gf/bbox3d.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#endif

#include <string>
#include <vector>

namespace lab {

class SelectionProvider : public Provider {
    struct data;
    data* _self;
    static SelectionProvider* _instance;
public:
    SelectionProvider();
    ~SelectionProvider();

    enum class SelectionType {
        Prim,
    };

    static SelectionProvider* instance();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Selection"; }

#ifndef HAVE_NO_USD
    std::vector<pxr::UsdPrim> GetSelectionPrims();
    std::vector<pxr::SdfPath> GetSelectionPaths();

    // Stage is used to resolve paths to prims so that the two are kept in sync
    void SetSelectionPrims(pxr::UsdStage*, const std::vector<pxr::UsdPrim>&);
    void SetSelectionPaths(pxr::UsdStage*, const std::vector<pxr::SdfPath>&);
#endif

    // in the future, selection will manage different kinds of selections
    void ClearSelection(SelectionType);
    int ItemCount(SelectionType) const;
    int Generation(SelectionType) const;
};

} // lab
#endif // Providers_Selection_hpp
