;; Scheme Shader System Implementation

;; Shader record type definition
(define-record-type shader
  (make-shader name inputs uniforms varyings body)
  shader?
  (name shader-name)
  (inputs shader-inputs)
  (uniforms shader-uniforms)
  (varyings shader-varyings)
  (body shader-body))

;; String utilities for GLSL code generation
(define (string-join strings delimiter)
  (if (null? strings)
      ""
      (let loop ((result (car strings))
                 (rest (cdr strings)))
        (if (null? rest)
            result
            (loop (string-append result delimiter (car rest))
                  (cdr rest))))))

;; Expression transpiler - converts Scheme expressions to GLSL
(define (transpile-expr expr)
  (cond
    ;; Literals
    ((number? expr) (number->string expr))
    ((symbol? expr) (symbol->string expr))
    
    ;; Function calls and operations
    ((pair? expr)
     (case (car expr)
       ;; Vector constructors
       ((vec2)
        (string-append "vec2(" 
                       (string-join (map transpile-expr (cdr expr)) ", ")
                       ")"))
       ((vec3)
        (string-append "vec3(" 
                       (string-join (map transpile-expr (cdr expr)) ", ")
                       ")"))
       ((vec4)
        (string-append "vec4(" 
                       (string-join (map transpile-expr (cdr expr)) ", ")
                       ")"))
       
       ;; Basic operators
       ((+)
        (string-append "(" 
                       (string-join (map transpile-expr (cdr expr)) " + ")
                       ")"))
       ((-)
        (if (= (length (cdr expr)) 1)
            (string-append "(-" (transpile-expr (cadr expr)) ")")
            (string-append "(" 
                          (string-join (map transpile-expr (cdr expr)) " - ")
                          ")")))
       ((*)
        (string-append "(" 
                       (string-join (map transpile-expr (cdr expr)) " * ")
                       ")"))
       ((/)
        (string-append "(" 
                       (string-join (map transpile-expr (cdr expr)) " / ")
                       ")"))
       
       ;; GLSL built-in functions
       ((dot normalize mix clamp pow max min)
        (string-append (symbol->string (car expr)) 
                       "(" 
                       (string-join (map transpile-expr (cdr expr)) ", ")
                       ")"))
       
       ;; Function calls (assume it's a user-defined function)
       (else
        (string-append (symbol->string (car expr))
                      "("
                      (string-join (map transpile-expr (cdr expr)) ", ")
                      ")"))))
    
    ;; Default case
    (else (error "Unknown expression type" expr))))

;; Statement transpiler - converts Scheme statements to GLSL
(define (transpile-statement stmt indent)
  (let ((indent-str (make-string (* indent 2) #\space)))
    (cond
      ;; Variable declaration
      ((and (pair? stmt) (eq? (car stmt) 'define))
       (string-append indent-str
                      "float " (symbol->string (cadr stmt)) " = "
                      (transpile-expr (caddr stmt)) ";"))
      
      ;; Let expressions become variable declarations
      ((and (pair? stmt) (eq? (car stmt) 'let))
       (let* ((bindings (cadr stmt))
              (body (cddr stmt))
              (declarations
                (map (lambda (binding)
                       (string-append indent-str 
                                      "float " (symbol->string (car binding))
                                      " = " (transpile-expr (cadr binding)) ";"))
                     bindings))
              (body-stmts
                (map (lambda (stmt)
                       (transpile-statement stmt (+ indent 1)))
                     (butlast body)))
              (return-stmt
                (string-append indent-str
                              "return " (transpile-expr (last body)) ";")))
         (string-join
           (append declarations body-stmts (list return-stmt))
           "\n")))
      
      ;; Let* expressions (sequential bindings)
      ((and (pair? stmt) (eq? (car stmt) 'let*))
       (let* ((bindings (cadr stmt))
              (body (cddr stmt))
              (declarations
                (map (lambda (binding)
                       (string-append indent-str 
                                      "float " (symbol->string (car binding))
                                      " = " (transpile-expr (cadr binding)) ";"))
                     bindings))
              (body-stmts
                (map (lambda (stmt)
                       (transpile-statement stmt (+ indent 1)))
                     (butlast body)))
              (return-stmt
                (string-append indent-str
                              "return " (transpile-expr (last body)) ";")))
         (string-join
           (append declarations body-stmts (list return-stmt))
           "\n")))
      
      ;; If expressions
      ((and (pair? stmt) (eq? (car stmt) 'if))
       (string-append indent-str
                      "if (" (transpile-expr (cadr stmt)) ") {\n"
                      (transpile-statement (caddr stmt) (+ indent 1)) "\n"
                      indent-str "} else {\n"
                      (transpile-statement (cadddr stmt) (+ indent 1)) "\n"
                      indent-str "}"))
      
      ;; Expression statement (default)
      (else
       (string-append indent-str (transpile-expr stmt) ";")))))

;; Helper functions for transpiling the shader body
(define (transpile-shader-body expr)
  (cond
    ;; Let/Let* expressions get special handling
    ((and (pair? expr) (or (eq? (car expr) 'let) (eq? (car expr) 'let*)))
     (let* ((bindings (cadr expr))
            (body (cddr expr))
            (var-decls 
              (map (lambda (binding)
                     (string-append "  " (type-for-expr (cadr binding)) " " 
                                   (symbol->string (car binding)) " = "
                                   (transpile-expr (cadr binding)) ";"))
                   bindings))
            (final-expr (last body)))
       (string-append
         (string-join var-decls "\n")
         "\n\n  gl_FragColor = " (transpile-expr final-expr) ";")))
    
    ;; Default case - just transpile the expression
    (else
     (string-append "  gl_FragColor = " (transpile-expr expr) ";"))))

;; Type inference for GLSL (simplified)
(define (type-for-expr expr)
  (cond
    ((number? expr) 
     (if (integer? expr) "int" "float"))
    ((symbol? expr) "float") ;; Default assumption
    ((pair? expr)
     (case (car expr)
       ((vec2) "vec2")
       ((vec3) "vec3")
       ((vec4) "vec4")
       ((mat2) "mat2")
       ((mat3) "mat3")
       ((mat4) "mat4")
       ;; For operations, determine based on operands
       ((+ - * /) 
        (let ((operand-types (map type-for-expr (cdr expr))))
          (cond
            ((member "vec4" operand-types) "vec4")
            ((member "vec3" operand-types) "vec3")
            ((member "vec2" operand-types) "vec2")
            ((member "float" operand-types) "float")
            (else "int"))))
       (else "float"))) ;; Default for unknown functions
    (else "float")))

;; Complete the transpile-to-glsl function to convert full shader
(define (complete-transpile-to-glsl shader)
  (let* ((name (shader-name shader))
         (inputs (shader-inputs shader))
         (uniforms (shader-uniforms shader))
         (varyings (shader-varyings shader))
         (body-fn (shader-body shader))
         (body-expr (body-fn)))
    
    (string-append
      "// Auto-generated GLSL shader from Scheme\n"
      "precision highp float;\n\n"
      
      ;; Generate uniform declarations
      (string-join 
        (map (lambda (u) 
             (string-append "uniform " 
                           (symbol->string (cadr u)) " " 
                           (symbol->string (car u)) ";"))
           uniforms)
        "\n")
      "\n\n"
      
      ;; Generate varying declarations
      (string-join 
        (map (lambda (v)
             (string-append "varying " 
                           (symbol->string (cadr v)) " " 
                           (symbol->string (car v)) ";"))
           varyings)
        "\n")
      "\n\n"
      
      ;; Generate constants for shader inputs
      (string-join 
        (map (lambda (i)
             (string-append "const " 
                           (symbol->string (cadr i)) " " 
                           (symbol->string (car i)) " = " 
                           (transpile-expr (caddr i)) ";"))
           inputs)
        "\n")
      "\n\n"
      
      ;; Generate helper functions (could be extracted from the shader)
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
      
      ;; Main function
      "void main() {\n"
      (transpile-shader-body body-expr)
      "\n}\n")))

;; Helper to get the last element of a list
(define (last lst)
  (if (null? (cdr lst))
      (car lst)
      (last (cdr lst))))

;; Helper to get all but the last element of a list
(define (butlast lst)
  (if (null? (cdr lst))
      '()
      (cons (car lst) (butlast (cdr lst)))))

