
#include "ShaderTranspiler.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
namespace shader {

// Helper function to initialize S7 Scheme environment
void initializeS7Environment(s7_scheme* s7) {
    // Define basic Scheme environment functions

    // Add a load-with-error-handling function
    std::string load_func_def = R"(
    (define (load-with-error-handling filename)
      (let ((content (with-input-from-file filename 
                      (lambda () (let loop ((lines '())
                                            (line (read-line)))
                                   (if (eof-object? line)
                                       (string-append (apply string-append (reverse lines)))
                                       (loop (cons (string-append line "\n") lines)
                                             (read-line))))))))
        (let ((port (open-input-string content)))
          (let loop ((result '()))
            (let ((expr (read port)))
              (if (eof-object? expr)
                  (reverse result)
                  (loop (cons (catch #t
                                (lambda () (eval expr))
                                (lambda (type info)
                                  (begin
                                    (display "Error in ")
                                    (display filename)
                                    (display ": ")
                                    (display info)
                                    (newline)
                                    'error)))
                              result))))))))
    )";

    s7_eval_c_string(s7, load_func_def.c_str());

    // Add other utility functions as needed
}

bool SchemeShaderTranspiler::loadSchemeFile(const std::string& filename) {
    return s7_load(m_s7, filename.c_str());
}


SchemeShaderTranspiler::SchemeShaderTranspiler(s7_scheme* s7, const std::string& shaderSystemPath)
: m_s7(s7)
{
    initializeS7Environment(m_s7);

    // First, define the function
    std::string define_test = "(define (test-function2 x) (* x 2))";
    evalScheme(define_test);

    // Then test it separately
    std::string test_result = evalScheme("(if (procedure? test-function) \"Success\" \"Failure\")");
    std::cout << "Test result: " << test_result << std::endl;

    // Try calling the function
    std::string call_result = evalScheme("(test-function 21)");
    std::cout << "Function call result: " << call_result << std::endl;

    // Define make-shader
    std::string define_make_shader = "(define (make-shader name inputs uniforms varyings body-fn) (list 'shader name inputs uniforms varyings body-fn))";
    evalScheme(define_make_shader);

    // Test if it's defined
    std::string shader_test = evalScheme("(if (procedure? make-shader) \"make-shader is defined\" \"make-shader is NOT defined\")");
    std::cout << "make-shader test: " << shader_test << std::endl;

    // Try using it
    std::string use_result = evalScheme("(make-shader 'test '() '() '() (lambda () 'dummy))");
    std::cout << "Using make-shader: " << use_result << std::endl;
    
    // Load shader system
    loadSchemeFile(shaderSystemPath);
}


std::string SchemeShaderTranspiler::evalScheme(const std::string& code) {
    if (!code.size()) {
        return "";
    }
    auto strport = s7_open_output_string(m_s7);
    auto prevport = s7_current_output_port(m_s7);
    auto strerrport = s7_open_output_string(m_s7);
    auto preverrport = s7_current_error_port(m_s7);

    std::string output;
    std::string err;
    std::string result;

    s7_set_current_output_port(m_s7, strport);
    s7_set_current_error_port(m_s7, strerrport);

    auto ret = s7_eval_c_string(m_s7, code.c_str());
    result = s7_object_to_c_string(m_s7, ret);

    output = s7_get_output_string(m_s7, strport);
    err = s7_get_output_string(m_s7, strerrport);

    s7_close_output_port(m_s7, strport);
    s7_close_output_port(m_s7, strerrport);

    s7_set_current_output_port(m_s7, prevport);
    s7_set_current_error_port(m_s7, preverrport);
    return result;
}



} // namespace shader

