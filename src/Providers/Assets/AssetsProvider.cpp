#include "AssetsProvider.hpp"
#include "Lab/LabDirectories.h"

namespace lab {
struct AssetsProvider::data {
};

AssetsProvider::AssetsProvider() : Provider(AssetsProvider::sname()) {
    _self = new AssetsProvider::data();
}

AssetsProvider::~AssetsProvider()
{
    delete _self;
}

AssetsProvider* AssetsProvider::instance()
{
    static AssetsProvider* _instance = new AssetsProvider();
    return _instance;
}

std::string AssetsProvider::resolve(const std::string& path) {
    // if the path contains $(LAB_BUNDLE) then replace it with the bundle path
    std::string result = path;
    if (result.find("$(LAB_BUNDLE)") != std::string::npos) {
        const char* bundle_path = lab_application_resource_path(nullptr, nullptr);
        result = result.replace(result.find("$(LAB_BUNDLE)"), 13, bundle_path);
    }
    return result;
}

} // namespace lab
