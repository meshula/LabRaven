
#include "SchemeProvider.hpp"
#include "ShaderTranspiler.hpp"
#include "s7-extensions.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <set>
#include <string>

namespace lab {


struct SchemeProvider::data {
    data() {
        s7 = s7_init();
        addS7Extensions(s7);
    }
    ~data() {
        s7_free(s7);
    }

    std::string schemeTranspilerPath;
    s7_scheme* s7;
};

SchemeProvider::SchemeProvider() : Provider(SchemeProvider::sname()) {
    _self = new SchemeProvider::data();
    _instance = this;
    provider.Documentation = [](void* instance) -> const char* {
        return "Provides a scheme interpreter";
    };
    
    // Set the path to the shader transpiler script
    _self->schemeTranspilerPath = "src/Providers/Scheme/shader-transpiler.scm";
}

SchemeProvider::~SchemeProvider() {
    delete _self;
}

SchemeProvider* SchemeProvider::_instance = nullptr;
SchemeProvider* SchemeProvider::instance() {
    if (!_instance)
        new SchemeProvider();
    return _instance;
}


std::string SchemeProvider::EvalScheme(const std::string& code) {
    if (!code.size()) {
        return "";
    }
    auto strport = s7_open_output_string(_self->s7);
    auto prevport = s7_current_output_port(_self->s7);
    auto strerrport = s7_open_output_string(_self->s7);
    auto preverrport = s7_current_error_port(_self->s7);

    std::string output;
    std::string err;
    std::string result;

    s7_set_current_output_port(_self->s7, strport);
    s7_set_current_error_port(_self->s7, strerrport);

    auto ret = s7_eval_c_string(_self->s7, code.c_str());
    result = s7_object_to_c_string(_self->s7, ret);

    output = s7_get_output_string(_self->s7, strport);
    err = s7_get_output_string(_self->s7, strerrport);

    s7_close_output_port(_self->s7, strport);
    s7_close_output_port(_self->s7, strerrport);

    s7_set_current_output_port(_self->s7, prevport);
    s7_set_current_error_port(_self->s7, preverrport);
    return result;
}

void SchemeProvider::ExerciseShaderTranspiler(const std::string& systemPath, const std::string& shaderPath) {
    try {
        // Create the extended transpiler
        shader::SchemeShaderTranspiler transpiler(_self->s7, systemPath);
        
        // Load shader definitions with pre-processing
        bool success = transpiler.loadSchemeFile(shaderPath);

        if (!success) {
            std::cerr << "Failed to load shader definitions" << std::endl;
            return;
        }
        
        std::cout << "Transpiling Disney Principled BRDF shader to GLSL...\n";
        
        // Transpile the shader to GLSL
        std::string glslCode = transpiler.evalScheme("(complete-transpile-to-glsl disney-principled-brdf)");
        
        // Check if result is valid
        if (glslCode.empty() || glslCode.find("Error") != std::string::npos) {
            std::cerr << "Transpilation failed: " << glslCode << std::endl;
        } else {
            // Save the generated shader to a file
            std::ofstream outFile("disney_brdf.glsl");
            outFile << glslCode;
            outFile.close();
            
            std::cout << "Shader successfully written to disney_brdf.glsl" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

} // lab
