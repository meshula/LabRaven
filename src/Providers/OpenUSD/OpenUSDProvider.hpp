#ifndef Provider_OpenUSDProvider_hpp
#define Provider_OpenUSDProvider_hpp

#include <memory>
#include <string>

namespace lab {

class OpenUSDProvider {
    struct Self;
    std::unique_ptr<Self> self;
public:
    OpenUSDProvider();
    ~OpenUSDProvider();
    void LoadStage(std::string const& filePath);
    void SaveStage();
    void ExportStage(std::string const& path);
};

} // lab

#endif
