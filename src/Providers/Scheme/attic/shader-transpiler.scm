;; Shader Language Abstraction System
;; A generic framework for transpiling shaders to multiple target languages

;; Define a data structure for shader languages
(define-record-type shader-language
  (make-shader-language 
   id                      ; Language identifier (e.g., 'glsl, 'msl)
   type-map                ; Mapping from our type system to language types
   function-map            ; Mapping of function names
   vector-constructor      ; Function to generate vector constructors
   entry-point-generator   ; Function to generate shader entry point
   uniform-generator       ; Function to generate uniform declarations
   varying-generator       ; Function to generate varying declarations
   constant-generator      ; Function to generate constant declarations
   helpers-generator)      ; Function to generate helper functions
  shader-language?
  (id shader-language-id)
  (type-map shader-language-type-map)
  (function-map shader-language-function-map)
  (vector-constructor shader-language-vector-constructor)
  (entry-point-generator shader-language-entry-point-generator)
  (uniform-generator shader-language-uniform-generator)
  (varying-generator shader-language-varying-generator)
  (constant-generator shader-language-constant-generator)
  (helpers-generator shader-language-helpers-generator))

;; Registry of shader languages
(define *shader-languages* '())

;; Register a shader language
(define (register-shader-language! language)
  (set! *shader-languages* 
        (cons (cons (shader-language-id language) language)
              (remove (lambda (pair) 
                       (eq? (car pair) (shader-language-id language)))
                     *shader-languages*))))

;; Get a shader language by ID
(define (get-shader-language id)
  (let ((pair (assq id *shader-languages*)))
    (if pair
        (cdr pair)
        (error "Unknown shader language" id))))

;; String utilities for code generation
(define (string-join strings delimiter)
  (if (null? strings)
      ""
      (let loop ((result (car strings))
                 (rest (cdr strings)))
        (if (null? rest)
            result
            (loop (string-append result delimiter (car rest))
                  (cdr rest))))))

;; Helper to get the last element of a list
(define (last lst)
  (if (null? (cdr lst))
      (car lst)
      (last (cdr lst))))

;; Generic transpiler using the shader language definitions
(define (complete-transpile language-id shader)
  (let* ((language (get-shader-language language-id))
         (name (shader-name shader))
         (inputs (shader-inputs shader))
         (uniforms (shader-uniforms shader))
         (varyings (shader-varyings shader))
         (body-fn (shader-body shader))
         (body-expr (body-fn)))
    
    (string-append
      ;; Header comments and language-specific preamble
      (string-append "// Auto-generated " 
                    (symbol->string language-id) 
                    " shader from Scheme\n")
      ((shader-language-entry-point-generator language))
      
      ;; Generate uniform declarations
      ((shader-language-uniform-generator language) uniforms)
      
      ;; Generate varying declarations
      ((shader-language-varying-generator language) varyings)
      
      ;; Generate constants for shader inputs
      ((shader-language-constant-generator language) inputs)
      
      ;; Generate helper functions
      ((shader-language-helpers-generator language) shader)
      
      ;; Transpile the shader body using language-specific rules
      (transpile-shader-body language body-expr inputs uniforms varyings))))

;; Transpile expressions with language-specific rules
(define (transpile-expr language expr)
  (let ((type-map (shader-language-type-map language))
        (function-map (shader-language-function-map language))
        (vector-constructor (shader-language-vector-constructor language)))
    
    (cond
      ;; Literals
      ((number? expr) (number->string expr))
      ((symbol? expr) (symbol->string expr))
      
      ;; Function calls and operations
      ((pair? expr)
       (case (car expr)
         ;; Vector constructors
         ((vec2 vec3 vec4)
          (vector-constructor (car expr) (cdr expr)))
         
         ;; Basic operators
         ((+ - * /)
          (string-append "(" 
                        (string-join (map (lambda (e) (transpile-expr language e)) 
                                        (cdr expr)) 
                                    (string-append " " (symbol->string (car expr)) " "))
                        ")"))
         
         ;; Function calls - use function mapping if available
         (else
          (let ((func-name (car expr))
                (args (cdr expr)))
            (let ((mapped-name (or (assq-ref function-map func-name)
                                  (symbol->string func-name))))
              (string-append mapped-name
                            "("
                            (string-join (map (lambda (e) 
                                             (transpile-expr language e)) 
                                           args) 
                                        ", ")
                            ")"))))))
      
      ;; Default case
      (else (error "Unknown expression type" expr)))))

;; Helper for association list lookups with default
(define (assq-ref alist key . default)
  (let ((pair (assq key alist)))
    (if pair
        (cdr pair)
        (if (null? default) #f (car default)))))

;; Transpile shader body with language-specific rules
(define (transpile-shader-body language expr inputs uniforms varyings)
  (cond
    ;; Let/Let* expressions
    ((and (pair? expr) (or (eq? (car expr) 'let) (eq? (car expr) 'let*)))
     (let* ((bindings (cadr expr))
            (body (cddr expr))
            (var-decls 
              (map (lambda (binding)
                     (string-append "    " 
                                   (infer-type-str language (cadr binding)) " " 
                                   (symbol->string (car binding)) " = "
                                   (transpile-expr language (cadr binding)) ";"))
                   bindings))
            (final-expr (last body)))
       (string-append
         (string-join var-decls "\n")
         "\n\n    return " (transpile-expr language final-expr) ";")))
    
    ;; Default case - just transpile the expression
    (else
     (string-append "    return " (transpile-expr language expr) ";"))))

;; Infer type string from expression for a specific language
(define (infer-type-str language expr)
  (let ((type-map (shader-language-type-map language)))
    (let ((type-sym (infer-type expr)))
      (or (assq-ref type-map type-sym)
          (symbol->string type-sym)))))

;; Generic type inference (language agnostic)
(define (infer-type expr)
  (cond
    ((number? expr) 
     (if (integer? expr) 'int 'float))
    ((symbol? expr) 'float) ;; Default assumption
    ((pair? expr)
     (case (car expr)
       ((vec2) 'vec2)
       ((vec3) 'vec3)
       ((vec4) 'vec4)
       ((mat2) 'mat2)
       ((mat3) 'mat3)
       ((mat4) 'mat4)
       ;; For operations, determine based on operands
       ((+ - * /) 
        (let ((operand-types (map infer-type (cdr expr))))
          (cond
            ((member 'vec4 operand-types) 'vec4)
            ((member 'vec3 operand-types) 'vec3)
            ((member 'vec2 operand-types) 'vec2)
            ((member 'float operand-types) 'float)
            (else 'int))))
       (else 'float))) ;; Default for unknown functions
    (else 'float)))

;; Define GLSL language
(define glsl-language
  (make-shader-language
   'glsl
   ;; Type mapping
   '((float . "float")
     (vec2 . "vec2")
     (vec3 . "vec3")
     (vec4 . "vec4")
     (mat2 . "mat2")
     (mat3 . "mat3")
     (mat4 . "mat4")
     (sampler2D . "sampler2D")
     (samplerCube . "samplerCube"))
   
   ;; Function mapping
   '((mix . "mix")
     (lerp . "mix")
     (dot . "dot")
     (cross . "cross")
     (normalize . "normalize")
     (pow . "pow")
     (sqrt . "sqrt")
     (min . "min")
     (max . "max")
     (clamp . "clamp"))
   
   ;; Vector constructor
   (lambda (type args)
     (string-append (symbol->string type) "(" 
                   (string-join (map (lambda (arg) 
                                     (transpile-expr glsl-language arg)) 
                                   args) 
                              ", ")
                   ")"))
   
   ;; Entry point generator
   (lambda ()
     "precision highp float;\n\n")
   
   ;; Uniform generator
   (lambda (uniforms)
     (string-append
       (string-join 
         (map (lambda (u) 
              (string-append "uniform " 
                            (assq-ref (shader-language-type-map glsl-language) (cadr u)
                                     (symbol->string (cadr u))) " " 
                            (symbol->string (car u)) ";"))
            uniforms)
         "\n")
       "\n\n"))
   
   ;; Varying generator
   (lambda (varyings)
     (string-append
       (string-join 
         (map (lambda (v)
              (string-append "varying " 
                            (assq-ref (shader-language-type-map glsl-language) (cadr v)
                                     (symbol->string (cadr v))) " " 
                            (symbol->string (car v)) ";"))
            varyings)
         "\n")
       "\n\n"))
   
   ;; Constant generator
   (lambda (inputs)
     (string-append
       (string-join 
         (map (lambda (i)
              (string-append "const " 
                            (assq-ref (shader-language-type-map glsl-language) (cadr i)
                                     (symbol->string (cadr i))) " " 
                            (symbol->string (car i)) " = " 
                            (transpile-expr glsl-language (caddr i)) ";"))
            inputs)
         "\n")
       "\n\n"))
   
   ;; Helper functions generator
   (lambda (shader)
     (string-append
       "// BRDF helper functions\n"
       "float ggxDistribution(float NdotH, float roughness) {\n"
       "    float alpha2 = roughness * roughness;\n"
       "    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;\n"
       "    return alpha2 / (3.14159265 * denom * denom);\n"
       "}\n\n"
       
       "float smithGGXGeometry(float NdotV, float NdotL, float roughness) {\n"
       "    float k = roughness * 0.5;\n"
       "    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);\n"
       "    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);\n"
       "    return ggx1 * ggx2;\n"
       "}\n\n"
       
       "vec3 fresnelSchlick(vec3 F0, float HdotV) {\n"
       "    return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);\n"
       "}\n\n"
       
       "void main() {\n"
       "    // Main shader implementation\n"))))

;; Define MSL language
(define msl-language
  (make-shader-language
   'msl
   ;; Type mapping
   '((float . "float")
     (vec2 . "float2")
     (vec3 . "float3")
     (vec4 . "float4")
     (mat2 . "float2x2")
     (mat3 . "float3x3")
     (mat4 . "float4x4")
     (sampler2D . "texture2d<float>")
     (samplerCube . "texturecube<float>"))
   
   ;; Function mapping
   '((mix . "mix")
     (lerp . "mix")
     (dot . "dot")
     (cross . "cross")
     (normalize . "normalize")
     (pow . "pow")
     (sqrt . "sqrt")
     (min . "min")
     (max . "max")
     (clamp . "clamp"))
   
   ;; Vector constructor
   (lambda (type args)
     (let ((msl-type (case type
                       ((vec2) "float2")
                       ((vec3) "float3")
                       ((vec4) "float4")
                       (else (symbol->string type)))))
       (string-append msl-type "(" 
                     (string-join (map (lambda (arg) 
                                       (transpile-expr msl-language arg)) 
                                     args) 
                                ", ")
                     ")")))
   
   ;; Entry point generator
   (lambda ()
     "#include <metal_stdlib>\nusing namespace metal;\n\n")
   
   ;; Uniform generator
   (lambda (uniforms)
     (string-append
       "struct Uniforms {\n"
       (string-join 
         (map (lambda (u) 
              (string-append "    " 
                            (assq-ref (shader-language-type-map msl-language) (cadr u)
                                     (symbol->string (cadr u))) " " 
                            (symbol->string (car u)) ";"))
            uniforms)
         "\n")
       "\n};\n\n"))
   
   ;; Varying generator
   (lambda (varyings)
     (string-append
       "struct VertexOutput {\n"
       "    float4 position [[position]];\n"
       (string-join 
         (map (lambda (v)
              (string-append "    " 
                            (assq-ref (shader-language-type-map msl-language) (cadr v)
                                     (symbol->string (cadr v))) " " 
                            (symbol->string (car v)) ";"))
            varyings)
         "\n")
       "\n};\n\n"))
   
   ;; Constant generator
   (lambda (inputs)
     (string-append
       "// Constants (shader parameters)\n"))
   
   ;; Helper functions generator
   (lambda (shader)
     (string-append
       "// BRDF helper functions\n"
       "float ggxDistribution(float NdotH, float roughness) {\n"
       "    float alpha2 = roughness * roughness;\n"
       "    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;\n"
       "    return alpha2 / (3.14159265 * denom * denom);\n"
       "}\n\n"
       
       "float smithGGXGeometry(float NdotV, float NdotL, float roughness) {\n"
       "    float k = roughness * 0.5;\n"
       "    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);\n"
       "    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);\n"
       "    return ggx1 * ggx2;\n"
       "}\n\n"
       
       "float3 fresnelSchlick(float3 F0, float HdotV) {\n"
       "    return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);\n"
       "}\n\n"
       
       "fragment float4 fragmentShader(VertexOutput in [[stage_in]],\n"
       "                               constant Uniforms& uniforms [[buffer(0)]]) {\n"))))

;; Register the languages
(register-shader-language! glsl-language)
(register-shader-language! msl-language)

;; Generic transpilation function using our framework
(define (transpile shader target-language)
  (complete-transpile target-language shader))

;; Test with a simple shader example
(define-shader test-shader
  ;; Inputs
  ((shader-input 'baseColor 'vec3 (vec3 0.8 0.8 0.8))
   (shader-input 'roughness 'float 0.5))
  
  ;; Uniforms
  ((uniform 'lightPosition 'vec3)
   (uniform 'cameraPosition 'vec3))
  
  ;; Varyings
  ((varying 'fragmentPosition 'vec3)
   (varying 'fragmentNormal 'vec3))
  
  ;; Body - simplified lighting calculation
  (let* ((normal (normalize fragmentNormal))
         (view-dir (normalize (- cameraPosition fragmentPosition)))
         (light-dir (normalize (- lightPosition fragmentPosition)))
         (half-vec (normalize (+ view-dir light-dir)))
         
         (n-dot-l (max (dot normal light-dir) 0.0))
         (n-dot-v (max (dot normal view-dir) 0.001))
         (n-dot-h (max (dot normal half-vec) 0.0))
         
         ;; Simple diffuse and specular terms
         (diffuse (* baseColor n-dot-l))
         (specular (* (pow n-dot-h (/ 2.0 roughness)) 0.5))
         
         ;; Final color
         (final-color (+ diffuse specular)))
    
    (vec4 final-color 1.0)))

;; Usage examples:
;; (transpile test-shader 'glsl)
;; (transpile test-shader 'msl)