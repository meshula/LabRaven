#ifndef AssetsActivity_hpp
#define AssetsActivity_hpp

#include "Lab/StudioCore.hpp"
#include <string>

namespace lab {
class AssetsProvider : public Provider
{
    struct data;
    data* _self;
public:
    explicit AssetsProvider();
    virtual ~AssetsProvider() override;

    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Assets"; }
    static AssetsProvider* instance();

    //--------------------------------------------------------------------------------
    std::string resolve(const std::string& path);
};

} // namespace lab

#endif
