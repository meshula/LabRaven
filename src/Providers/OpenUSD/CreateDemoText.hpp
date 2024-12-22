#ifndef CreateDemoText_hpp
#define CreateDemoText_hpp
#include "OpenUSDProvider.hpp"
#include <pxr/usd/sdf/path.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/usd/usd/stage.h>
#include <string>

namespace lab {

PXR_NAMESPACE_USING_DIRECTIVE

// Creates a 3D text mesh using the C64 font, where each character is made up of cubes
// Returns the path to the created text mesh
pxr::SdfPath CreateC64DemoText(OpenUSDProvider&, const std::string& text, PXR_NS::GfVec3d pos);

} // namespace lab

#endif
