#include "scheme_shader_transpiler.hpp"
#include <iostream>
#include <vector>
#include <string>

int main() {
    try {
        // Initialize the transpiler
        shader::SchemeShaderTranspiler transpiler("shader-system.scm");
        
        // Load predefined shaders
        transpiler.loadShaderDefinitions("shader-defs.scm");
        
        std::cout << "Transpiling Disney Principled BRDF shader to GLSL...\n";
        
        // Transpile the shader to GLSL
        std::string glslCode = transpiler.transpileShader("disney-principled-brdf");
        
        // Save the generated shader to a file
        transpiler.saveShaderToFile(glslCode, "disney_brdf.glsl");
        
        std::cout << "Creating modified version of the shader...\n";
        
        // Modify the roughness parameter
        std::string modifiedShader = transpiler.modifyShaderParam(
            "disney-principled-brdf", "roughness", {0.2f}
        );
        
        // Transpile the modified shader
        std::string modifiedGlsl = transpiler.transpileShader(modifiedShader);
        transpiler.saveShaderToFile(modifiedGlsl, "disney_brdf_modified.glsl");
        
        // Define a new shader directly from C++
        std::cout << "Defining a new Phong shader from C++...\n";
        
        std::vector<shader::ShaderParam> phongInputs = {
            {"diffuseColor", shader::ParamType::Vec3, {0.8f, 0.2f, 0.2f}},
            {"specularColor", shader::ParamType::Vec3, {1.0f, 1.0f, 1.0f}},
            {"shininess", shader::ParamType::Float, {32.0f}}
        };
        
        std::vector<shader::ShaderUniform> phongUniforms = {
            {"lightPosition", shader::ParamType::Vec3},
            {"cameraPosition", shader::ParamType::Vec3}
        };
        
        std::vector<shader::ShaderVarying> phongVaryings = {
            {"fragmentPosition", shader::ParamType::Vec3},
            {"fragmentNormal", shader::ParamType::Vec3}
        };
        
        // Phong shader body in Scheme syntax
        std::string phongBody = R"(
(let* ((normal (normalize fragmentNormal))
       (view-dir (normalize (- cameraPosition fragmentPosition)))
       (light-dir (normalize (- lightPosition fragmentPosition)))
       
       ;; Diffuse term
       (diff (max (dot normal light-dir) 0.0))
       (diffuse (* diffuseColor diff))
       
       ;; Specular term
       (reflect-dir (normalize (- light-dir (* 2.0 (dot light-dir normal) normal))))
       (spec (pow (max (dot view-dir reflect-dir) 0.0) shininess))
       (specular (* specularColor spec))
       
       ;; Ambient term
       (ambient (* diffuseColor 0.1))
       
       ;; Final color 
       (final-color (+ ambient diffuse specular)))
  
  (vec4 final-color 1.0))
)";
        
        // Define the Phong shader
        transpiler.defineShader("phong-shader", phongInputs, phongUniforms, phongVaryings, phongBody);
        
        // Transpile the Phong shader
        std::string phongGlsl = transpiler.transpileShader("phong-shader");
        transpiler.saveShaderToFile(phongGlsl, "phong_shader.glsl");
        
        std::cout << "Generated three shaders successfully.\n";
        
        // Demonstrate using the same system to generate HLSL
        // Note: This would require implementing the HLSL transpiler in Scheme
        // std::string hlslCode = transpiler.transpileShader("disney-principled-brdf", 
        //                                                  shader::ShaderLanguage::HLSL);
        // transpiler.saveShaderToFile(hlslCode, "disney_brdf.hlsl");
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}