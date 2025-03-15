#ifndef SCHEME_SHADER_TRANSPILER_HPP
#define SCHEME_SHADER_TRANSPILER_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <memory>

extern "C" {
#include "s7.h"
}

namespace shader {

// Shader parameter types
enum class ParamType {
    Float,
    Vec2,
    Vec3,
    Vec4,
    Int,
    Bool,
    Sampler2D,
    SamplerCube
};

// Shader parameter representation
struct ShaderParam {
    std::string name;
    ParamType type;
    std::vector<float> defaultValue;
};

// Shader uniform representation
struct ShaderUniform {
    std::string name;
    ParamType type;
};

// Shader varying representation
struct ShaderVarying {
    std::string name;
    ParamType type;
};

// Target shader languages
enum class ShaderLanguage {
    GLSL,
    HLSL,
    MSL // Metal Shading Language
};

/**
 * @class SchemeShaderTranspiler
 * @brief C++ wrapper for the Scheme-based shader transpiler
 */
class SchemeShaderTranspiler {
public:
    /**
     * @brief Constructor - initializes the S7 Scheme interpreter and loads shader system
     * @param shaderSystemPath Path to the shader-system.scm file
     */
    SchemeShaderTranspiler(s7_scheme* s7, const std::string& shaderSystemPath = "shader-system.scm");
    
    ~SchemeShaderTranspiler() = default;
    
    /**
     * @brief Load a Scheme file with shader definitions
     * @param filename Path to the Scheme file
     */
    void loadShaderDefinitions(const std::string& filename) {
        loadSchemeFile(filename);
    }
    
    /**
     * @brief Define a shader directly from C++ code
     * @param shaderName Name of the shader
     * @param inputs Shader parameters/inputs
     * @param uniforms Shader uniforms
     * @param varyings Shader varyings
     * @param schemeBody The shader body as a Scheme expression (string)
     */
    void defineShader(const std::string& shaderName,
                      const std::vector<ShaderParam>& inputs,
                      const std::vector<ShaderUniform>& uniforms,
                      const std::vector<ShaderVarying>& varyings,
                      const std::string& schemeBody) {
        // Convert the C++ structures to Scheme expressions
        std::string inputsStr = convertInputsToScheme(inputs);
        std::string uniformsStr = convertUniformsToScheme(uniforms);
        std::string varyingsStr = convertVaryingsToScheme(varyings);
        
        // Construct the full shader definition
        std::string shaderDef = 
            "(define-shader " + shaderName + "\n"
            "  ;; Inputs\n"
            "  (" + inputsStr + ")\n"
            "  ;; Uniforms\n"
            "  (" + uniformsStr + ")\n"
            "  ;; Varyings\n"
            "  (" + varyingsStr + ")\n"
            "  ;; Body\n"
            "  " + schemeBody + ")\n";
            
        // Evaluate the shader definition
        s7_pointer result = s7_eval_c_string(m_s7, shaderDef.c_str());
        checkForErrors(result);
    }
    
    /**
     * @brief Transpile a shader to a target language
     * @param shaderName Name of the shader to transpile
     * @param language Target shader language
     * @return Generated shader code as a string
     */
    std::string transpileShader(const std::string& shaderName, 
                              ShaderLanguage language = ShaderLanguage::GLSL) {
        // Get the shader object
        s7_pointer shader = s7_name_to_value(m_s7, shaderName.c_str());
        if (s7_is_null(m_s7, shader)) {
            throw std::runtime_error("Shader '" + shaderName + "' not found");
        }
        
        std::string transpilerFn;
        switch (language) {
            case ShaderLanguage::GLSL:
                transpilerFn = "complete-transpile-to-glsl";
                break;
            case ShaderLanguage::HLSL:
                transpilerFn = "transpile-to-hlsl";
                break;
            case ShaderLanguage::MSL:
                transpilerFn = "transpile-to-metal-shader-language";
                break;
        }
        
        // Call the transpiler function
        s7_pointer args = s7_list(m_s7, 1, shader);
        s7_pointer result = s7_call(m_s7, s7_name_to_value(m_s7, transpilerFn.c_str()), args);
        checkForErrors(result);
        
        return s7_string(result);
    }
    
    /**
     * @brief Save a transpiled shader to a file
     * @param shaderCode The transpiled shader code
     * @param filename Output filename
     */
    void saveShaderToFile(const std::string& shaderCode, 
                         const std::string& filename) {
        std::ofstream outFile(filename);
        if (!outFile) {
            throw std::runtime_error("Could not open file for writing: " + filename);
        }
        
        outFile << shaderCode;
        outFile.close();
        
        std::cout << "Shader successfully written to " << filename << std::endl;
    }
    
    /**
     * @brief Modify a shader parameter
     * @param shaderName Name of the shader to modify
     * @param paramName Name of the parameter to modify
     * @param newValue New value for the parameter
     * @return Name of the newly created modified shader
     */
    std::string modifyShaderParam(const std::string& shaderName,
                                const std::string& paramName,
                                const std::vector<float>& newValue) {
        // Create a unique name for the modified shader
        std::string modifiedName = shaderName + "_modified_" + paramName;
        
        // Convert the new value to a Scheme expression
        std::string valueExpr = convertValueToScheme(paramName, newValue);
        
        // Create the modification script
        std::string script = 
            "(define " + modifiedName + "\n"
            "  (make-shader\n"
            "    (shader-name " + shaderName + ")\n"
            "    (map (lambda (input)\n"
            "           (if (eq? (car input) '" + paramName + ")\n"
            "               (list '" + paramName + " (cadr input) " + valueExpr + ")\n"
            "               input))\n"
            "         (shader-inputs " + shaderName + "))\n"
            "    (shader-uniforms " + shaderName + ")\n"
            "    (shader-varyings " + shaderName + ")\n"
            "    (shader-body " + shaderName + ")))\n";
            
        // Evaluate the modification script
        s7_pointer result = s7_eval_c_string(m_s7, script.c_str());
        checkForErrors(result);
        
        return modifiedName;
    }
    
    /**
     * @brief Execute arbitrary Scheme code
     * @param code Scheme code to execute
     * @return Result as a string
     */
    std::string evalScheme(const std::string& code);
    /**
     * @brief Load and evaluate a Scheme file
     * @param filename Path to the Scheme file
     */
    bool loadSchemeFile(const std::string& filename);

protected:
    s7_scheme* m_s7;


    // Helper to escape special characters for Scheme strings
    std::string escapeForScheme(const std::string& str) {
        std::string result;
        for (char c : str) {
            if (c == '\"' || c == '\\') {
                result += '\\';
            }
            result += c;
        }
        return result;
    }

    void checkForErrors(s7_pointer result) {
        if (!result) {
            throw std::runtime_error("S7 evaluation returned null");
        }

        // Check the string representation of the result
        std::string resultStr = s7_object_to_c_string(m_s7, result);
        if (resultStr.find("error") != std::string::npos ||
            resultStr.find("Error") != std::string::npos ||
            resultStr.find("exception") != std::string::npos) {
            throw std::runtime_error("Scheme evaluation error: " + resultStr);
        }
    }
    /**
     * @brief Convert C++ shader inputs to Scheme syntax
     * @param inputs Vector of ShaderParam structures
     * @return Scheme expression as a string
     */
    std::string convertInputsToScheme(const std::vector<ShaderParam>& inputs) {
        std::string result;
        for (const auto& input : inputs) {
            if (!result.empty()) {
                result += "\n   ";
            }
            result += "(shader-input '" + input.name + " '" + 
                     paramTypeToString(input.type) + " " +
                     convertValueToScheme(input.name, input.defaultValue) + ")";
        }
        return result;
    }
    
    /**
     * @brief Convert C++ shader uniforms to Scheme syntax
     * @param uniforms Vector of ShaderUniform structures
     * @return Scheme expression as a string
     */
    std::string convertUniformsToScheme(const std::vector<ShaderUniform>& uniforms) {
        std::string result;
        for (const auto& uniform : uniforms) {
            if (!result.empty()) {
                result += "\n   ";
            }
            result += "(uniform '" + uniform.name + " '" + 
                     paramTypeToString(uniform.type) + ")";
        }
        return result;
    }
    
    /**
     * @brief Convert C++ shader varyings to Scheme syntax
     * @param varyings Vector of ShaderVarying structures
     * @return Scheme expression as a string
     */
    std::string convertVaryingsToScheme(const std::vector<ShaderVarying>& varyings) {
        std::string result;
        for (const auto& varying : varyings) {
            if (!result.empty()) {
                result += "\n   ";
            }
            result += "(varying '" + varying.name + " '" + 
                     paramTypeToString(varying.type) + ")";
        }
        return result;
    }
    
    /**
     * @brief Convert a parameter type enum to its string representation
     * @param type Parameter type
     * @return String representation of the type
     */
    std::string paramTypeToString(ParamType type) {
        switch (type) {
            case ParamType::Float: return "float";
            case ParamType::Vec2: return "vec2";
            case ParamType::Vec3: return "vec3";
            case ParamType::Vec4: return "vec4";
            case ParamType::Int: return "int";
            case ParamType::Bool: return "bool";
            case ParamType::Sampler2D: return "sampler2D";
            case ParamType::SamplerCube: return "samplerCube";
            default: return "float";
        }
    }
    
    /**
     * @brief Convert a C++ value to a Scheme expression
     * @param name Parameter name
     * @param value Parameter value
     * @return Scheme expression as a string
     */
    std::string convertValueToScheme(const std::string& name, 
                                   const std::vector<float>& value) {
        if (value.empty()) {
            return "0.0";
        }
        
        // Check if this is a vector type based on number of components
        if (value.size() >= 2 && value.size() <= 4) {
            std::string vecType = "vec" + std::to_string(value.size());
            std::string components;
            for (size_t i = 0; i < value.size(); ++i) {
                if (i > 0) components += " ";
                components += std::to_string(value[i]);
            }
            return "(" + vecType + " " + components + ")";
        }
        
        // Default to a single float
        return std::to_string(value[0]);
    }
};

} // namespace shader

#endif // SCHEME_SHADER_TRANSPILER_HPP
