
#ifndef Providers_SchemeProvider_hpp
#define Providers_SchemeProvider_hpp

#include "Lab/StudioCore.hpp"

namespace lab {

class SchemeProvider : public Provider {
    struct data;
    data* _self;
    static SchemeProvider* _instance;
public:
SchemeProvider();
    ~SchemeProvider();

    static SchemeProvider* instance();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Scheme"; }

    // shaderTranspiler.hpp should be used to 
    void ExerciseShaderTranspiler(const std::string& systemPath, const std::string& shaderPath);

    std::string EvalScheme(const std::string& code);
};

} // lab
#endif // Providers_SchemeProvider_hpp
