
#include "SelectionProvider.hpp"
#include <set>
#include <string>

namespace lab {

struct SelectionProvider::data {
#ifndef HAVE_NO_USD
    std::vector<pxr::UsdPrim> selectionPrims;
    std::vector<pxr::SdfPath> selectionPaths;
#endif
    int generation = 0;
};

SelectionProvider::SelectionProvider() : Provider(SelectionProvider::sname()) {
    _self = new SelectionProvider::data();
    _instance = this;
    provider.Documentation = [](void* instance) -> const char* {
        return "Maintains selections";
    };
}

SelectionProvider::~SelectionProvider() {
    delete _self;
}

SelectionProvider* SelectionProvider::_instance = nullptr;
SelectionProvider* SelectionProvider::instance() {
    if (!_instance)
        new SelectionProvider();
    return _instance;
}


#ifndef HAVE_NO_USD
std::vector<pxr::UsdPrim> SelectionProvider::GetSelectionPrims() {
    return _self->selectionPrims;
}
std::vector<pxr::SdfPath> SelectionProvider::GetSelectionPaths() {
    return _self->selectionPaths;
}

void SelectionProvider::SetSelectionPrims(pxr::UsdStage* stage, const std::vector<pxr::UsdPrim>& s) {
    _self->generation++;
    _self->selectionPrims = s;
    _self->selectionPaths.clear();
    for (auto prim : s) {
        _self->selectionPaths.push_back(prim.GetPath());
    }
}

void SelectionProvider::SetSelectionPaths(pxr::UsdStage* stage, const std::vector<pxr::SdfPath>& s) {
    _self->generation++;
    _self->selectionPrims.clear();
    _self->selectionPaths = s;
    for (auto path : s) {
        auto prim = stage->GetPrimAtPath(path);
        _self->selectionPrims.push_back(prim);
    }
}

#endif

void SelectionProvider::ClearSelection(SelectionType) {
    _self->generation++;
#ifndef HAVE_NO_USD
    _self->selectionPrims.clear();
    _self->selectionPaths.clear();
#endif
}

int SelectionProvider::ItemCount(SelectionType) const {
#ifndef HAVE_NO_USD
    return (int) _self->selectionPaths.size();
#endif
}

int SelectionProvider::Generation(SelectionType) const {
    return _self->generation;
}


} // lab
