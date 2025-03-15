;; Shader Language Abstraction System
;; A generic framework for transpiling shaders to multiple target languages

;; S7 Scheme doesn't support SRFI-9 define-record-type by default
;; Implementing a simpler record system compatible with S7

(define (debug message . args)
  (display "SHADER DEBUG: ")
  (display message)
  (for-each (lambda (arg)
              (display " ")
              (display arg))
            args)
  (newline))

(debug "Starting shader system load")

;; Create a shader function record
(define (make-shader-function name params return-type body)
  (list 'shader-function name params return-type body))

(debug "Defined make-shader-function")

(define (shader-function? obj)
  (and (pair? obj) (eq? (car obj) 'shader-function)))

(debug "Defined shader-function?")

(define (shader-function-name func)
  (cadr func))

(define (shader-function-params func)
  (caddr func))

(define (shader-function-return-type func)
  (cadddr func))

(define (shader-function-body func)
  (car (cddddr func)))

;; Create a shader record
(define (make-shader name inputs uniforms varyings body-fn)
  (list 'shader name inputs uniforms varyings body-fn))

(debug "Defined make-shader")

(define (shader? obj)
  (and (pair? obj) (eq? (car obj) 'shader)))

(define (shader-name shader)
  (cadr shader))

(define (shader-inputs shader)
  (caddr shader))

(define (shader-uniforms shader)
  (cadddr shader))

(define (shader-varyings shader)
  (car (cddddr shader)))

(define (shader-body shader)
  (cadr (cddddr shader)))

;; Since S7 might not fully support syntax-rules, using macros or procedures instead
(define (define-shader name inputs uniforms varyings . body)
  (define shader-obj 
    (make-shader name inputs uniforms varyings
                (lambda () (if (null? body) '() (car body)))))
  
  ;; Store the shader in a global variable with name as symbol
  (eval `(define ,name ',shader-obj) (interaction-environment))
  shader-obj)

;; Helper for defining shader inputs
(define (shader-input name type default)
  (list name type default))

;; Helper for defining uniforms
(define (uniform name type)
  (list name type))

;; Helper for defining varyings
(define (varying name type)
  (list name type))

;; Define a data structure for shader languages
(define (make-shader-language id type-map function-map 
                             vector-constructor entry-point-generator
                             uniform-generator varying-generator
                             constant-generator function-generator
                             main-generator)
  (list 'shader-language id type-map function-map
        vector-constructor entry-point-generator
        uniform-generator varying-generator 
        constant-generator function-generator
        main-generator))

(debug "Defined make-shader-language")


(define (shader-language? obj)
  (and (pair? obj) (eq? (car obj) 'shader-language)))

(define (shader-language-id lang)
  (cadr lang))

(define (shader-language-type-map lang)
  (caddr lang))

(define (shader-language-function-map lang)
  (cadddr lang))

(define (shader-language-vector-constructor lang)
  (car (cddddr lang)))

(define (shader-language-entry-point-generator lang)
  (cadr (cddddr lang)))

(define (shader-language-uniform-generator lang)
  (caddr (cddddr lang)))

(define (shader-language-varying-generator lang)
  (cadddr (cddddr lang)))

(define (shader-language-constant-generator lang)
  (car (cddddr (cddddr lang))))

(define (shader-language-helpers-generator lang)
  (cadr (cddddr (cddddr lang))))

(define (shader-language-main-generator lang)
  (caddr (cddddr (cddddr lang))))

;; Registry of shader languages
(define *shader-languages* '())

;; Register a shader language
(define (register-shader-language! language)
  (set! *shader-languages* 
        (cons (cons (shader-language-id language) language)
              (filter (lambda (pair) 
                        (not (eq? (car pair) (shader-language-id language))))
                     *shader-languages*))))

;; Get a shader language by ID
(define (get-shader-language id)
  (let ((pair (assoc id *shader-languages*)))
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

;; Helper for association list lookups with default
(define (assq-ref alist key . default)
  (let ((pair (assq key alist)))
    (if pair
        (cdr pair)
        (if (null? default) #f (car default)))))

;; Clean implementations of vector functions

;; Basic vec3 constructor
(define (vec3 x y z) 
  (list 'vec3 x y z))

;; vec3 component access
(define (vec3-x v)
  (if (and (pair? v) (eq? (car v) 'vec3))
      (cadr v)
      (list 'vec3-x v)))

(define (vec3-y v)
  (if (and (pair? v) (eq? (car v) 'vec3))
      (caddr v)
      (list 'vec3-y v)))

(define (vec3-z v)
  (if (and (pair? v) (eq? (car v) 'vec3))
      (cadddr v)
      (list 'vec3-z v)))

;; Flexible vec4 constructor that handles both forms
;; Form 1: (vec4 x y z w) - standard 4 scalar arguments
;; Form 2: (vec4 v3 w) - vec3 and scalar
(define (vec4 . args)
  (cond
    ;; 4 scalar arguments
    ((= (length args) 4)
     (list 'vec4 (car args) (cadr args) (caddr args) (cadddr args)))
    
    ;; vec3 + scalar
    ((= (length args) 2)
     (if (and (pair? (car args)) (eq? (caar args) 'vec3))
         ;; Explicit expansion if first arg is a literal vec3
         (list 'vec4 (cadar args) (caddar args) (cadddr (car args)) (cadr args))
         ;; Otherwise mark it as a vec3 + scalar case
         (list 'vec4-from-vec3 (car args) (cadr args))))
    
    ;; Error case
    (else
     (display "Error: vec4 requires either 4 scalar arguments or a vec3 and a scalar\n")
     (list 'vec4-error args))))

;; Helper function that's only used during transpilation
(define (vec4-from-vec3 v3 w)
  (list 'vec4-from-vec3 v3 w))

(define (dot a b) (list 'dot a b))
(define (normalize v) (list 'normalize v))
(define (mix a b t) (list 'mix a b t))
(define (clamp x min max) (list 'clamp x min max))
(define (pow base exp) (list 'pow base exp))
(define (max a b) (list 'max a b))
(define (min a b) (list 'min a b))

;; Basic math operations work as expected
(define (+ . args) (cons '+ args))
(define (* . args) (cons '* args))
(define (- . args) (cons '- args))
(define (/ . args) (cons '/ args))

;; Function to transpile a shader function to a specific language
(define (transpile-function language func)
  (let* ((name (shader-function-name func))
         (params (shader-function-params func))
         (return-type (shader-function-return-type func))
         (body (shader-function-body func))
         (type-map (shader-language-type-map language))
         
         ;; Determine return type string for this language
         (return-type-str (assq-ref type-map return-type 
                                   (symbol->string return-type)))
         
         ;; Generate parameter list
         (param-strings
          (map (lambda (param)
                (let* ((param-name (car param))
                       (param-type (cadr param))
                       (type-str (assq-ref type-map param-type
                                          (symbol->string param-type))))
                  (string-append type-str " " (symbol->string param-name))))
               params))
         
         ;; Transpile the function body
         (body-string (transpile-expr language body)))
    
    ;; Assemble the function definition
    (string-append
     return-type-str " " (symbol->string name) "("
     (string-join param-strings ", ")
     ") {\n"
     "    return " body-string ";\n"
     "}\n")))

;; Generic transpiler using the shader language definitions
(define (complete-transpile language-id shader)
  (let* ((language (get-shader-language language-id))
         (name (shader-name shader))
         (inputs (shader-inputs shader))
         (uniforms (shader-uniforms shader))
         (varyings (shader-varyings shader))
         (body-fn (shader-body shader)))
    
    ;; Add debugging before evaluating body function
    (display "About to evaluate shader body for: ")
    (display name)
    (newline)
    
    ;; Evaluate body function with error handling
    (let ((body-expr 
           (catch #t
              (lambda () (body-fn))
              (lambda (type info)
                (display "Error evaluating shader body: ")
                (display info)
                (newline)
                (list 'error-in-body)))))
      
      ;; Add debugging after evaluating body function
      (display "Shader body evaluated to: ")
      (display body-expr)
      (newline)
      
      ;; Only proceed if body evaluation was successful
      (if (and (pair? body-expr) (eq? (car body-expr) 'error-in-body))
          "// Error in shader body - transpilation aborted"
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
            
            ;; Generate main function wrapper with shader body
            ((shader-language-main-generator language)
             (transpile-shader-body language body-expr inputs uniforms varyings)))))))

;; Helper function for creating vec4 from vec3
(define (vec4-from-vec3 v3 w)
  (list 'vec4-from-vec3 v3 w))

;; Simplified transpile-expr with basic circular reference handling
(define (transpile-expr language expr)
  ;; Use a simple list to track the current expression path
  (define current-path '())
  
  (define (helper expr)
    ;; Check for circular references
    (if (member expr current-path)
        "/* Circular reference */"
        (begin
          ;; Add to current path
          (set! current-path (cons expr current-path))
          
          ;; Process the expression
          (let ((result
                 (cond
                   ;; Special case for vec4-from-vec3
                   ((and (pair? expr) (eq? (car expr) 'vec4-from-vec3))
                    (string-append "vec4(" (helper (cadr expr)) ", " (helper (caddr expr)) ")"))
                   
                   ;; Basic literals
                   ((number? expr) (number->string expr))
                   ((symbol? expr) (symbol->string expr))
                   ((string? expr) expr)
                   
                   ;; Compound expressions
                   ((pair? expr)
                    (case (car expr)
                      ;; Vector constructors
                      ((vec2 vec3 vec4)
                       (string-append (symbol->string (car expr)) "("
                                     (string-join (map helper (cdr expr)) ", ")
                                     ")"))
                      
                      ;; Operators
                      ((+ - * /)
                       (string-append "("
                                     (string-join (map helper (cdr expr))
                                                 (string-append " " (symbol->string (car expr)) " "))
                                     ")"))
                      
                      ;; General function calls
                      (else
                       (string-append (symbol->string (car expr)) "("
                                     (string-join (map helper (cdr expr)) ", ")
                                     ")"))))
                   
                   ;; Unknown expression type
                   (else "/* Unknown expression type */"))))
            
            ;; Remove from current path
            (set! current-path (cdr current-path))
            
            result))))
  
  ;; Start the helper with empty path
  (helper expr))

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

;; BRDF specific functions
(define (fresnel-schlick f0 h-dot-v)
  (+ f0 (* (- 1.0 f0) (pow (- 1.0 h-dot-v) 5.0))))

(define (ggx-distribution n-dot-h roughness)
  (let ((alpha-squared (* roughness roughness)))
    (/ alpha-squared
       (* 3.14159265
          (pow (+ (* n-dot-h n-dot-h (- alpha-squared 1.0)) 1.0) 2.0)))))

(define (smith-ggx-geometry n-dot-v n-dot-l roughness)
  (let ((k (* roughness 0.5)))
    (* (/ n-dot-v (+ n-dot-v (* (- 1.0 n-dot-v) k)))
       (/ n-dot-l (+ n-dot-l (* (- 1.0 n-dot-l) k))))))

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
   
   ;; Function generator - for helper functions
   (lambda (func)
     (transpile-function glsl-language func))
   
   ;; Main function generator
   (lambda (body-string)
     (string-append
      "void main() {\n"
      "    // Main shader implementation\n"
      body-string
      "\n}\n"))))

;; Add this function after your register-shader-language! function
(define (describe-shader-language language-id)
  (let ((language (get-shader-language language-id)))
    (if language
        (begin
          (display "Shader Language: ")
          (display (shader-language-id language))
          (newline)
          
          (display "  Type Mappings:\n")
          (for-each (lambda (pair)
                      (display "    ")
                      (display (car pair))
                      (display " -> ")
                      (display (cdr pair))
                      (newline))
                    (shader-language-type-map language))
          
          (display "  Function Mappings:\n")
          (for-each (lambda (pair)
                      (display "    ")
                      (display (car pair))
                      (display " -> ")
                      (display (cdr pair))
                      (newline))
                    (shader-language-function-map language))
          
          (display "  Has vector constructor: ")
          (display (if (procedure? (shader-language-vector-constructor language)) "yes" "no"))
          (newline)
          
          (display "  Has entry point generator: ")
          (display (if (procedure? (shader-language-entry-point-generator language)) "yes" "no"))
          (newline)
          
          (display "  Has uniform generator: ")
          (display (if (procedure? (shader-language-uniform-generator language)) "yes" "no"))
          (newline)
          
          (display "  Has varying generator: ")
          (display (if (procedure? (shader-language-varying-generator language)) "yes" "no"))
          (newline)
          
          (display "  Has constant generator: ")
          (display (if (procedure? (shader-language-constant-generator language)) "yes" "no"))
          (newline)
          
          (display "  Has helpers generator: ")
          (display (if (procedure? (shader-language-helpers-generator language)) "yes" "no"))
          (newline)
          
          (display "  Has main generator: ")
          (display (if (procedure? (shader-language-main-generator language)) "yes" "no"))
          (newline))
        
        (begin
          (display "Language not found: ")
          (display language-id)
          (newline)))))

;; Call it immediately after registering the language
(register-shader-language! glsl-language)
(display "===== Registered GLSL Language =====\n")
(describe-shader-language 'glsl)
(display "===================================\n")

;; Also add a function to list all registered languages
(define (list-registered-languages)
  (display "Registered Shader Languages:\n")
  (for-each (lambda (pair)
              (display "  ")
              (display (car pair))
              (newline))
            *shader-languages*))

;; Call it after all languages are registered
(list-registered-languages)

;; Helper to add a shader helper function to the standard library
(define (define-shader-helper name params return-type body)
  (let ((func (make-shader-function name params return-type body)))
    func))

;; Define standard helper functions
(define standard-helper-functions
  (list
   (define-shader-helper 'fresnel_schlick 
                       '((f0 float) (h_dot_v float)) 
                       'float 
                       '(+ f0 (* (- 1.0 f0) (pow (- 1.0 h_dot_v) 5.0))))
   
   (define-shader-helper 'ggx_distribution 
                       '((n_dot_h float) (roughness float)) 
                       'float 
                       '(let ((alpha_squared (* roughness roughness)))
                         (/ alpha_squared
                           (* 3.14159265
                             (pow (+ (* n_dot_h n_dot_h (- alpha_squared 1.0)) 1.0) 2.0)))))
   
   (define-shader-helper 'smith_ggx_geometry 
                       '((n_dot_v float) (n_dot_l float) (roughness float)) 
                       'float 
                       '(let ((k (* roughness 0.5)))
                         (* (/ n_dot_v (+ n_dot_v (* (- 1.0 n_dot_v) k)))
                           (/ n_dot_l (+ n_dot_l (* (- 1.0 n_dot_l) k))))))))

;; Function to complete transpile to glsl
(define (complete-transpile-to-glsl shader)
  (complete-transpile 'glsl shader))

;; Generic transpilation function using our framework
(define (transpile shader target-language)
  (complete-transpile target-language shader))


(define (describe-shader shader-obj-or-name)
  (let ((shader (if (symbol? shader-obj-or-name)
                    (begin
                      (display "Looking up shader by name: ")
                      (display shader-obj-or-name)
                      (newline)
                      (let ((val (eval shader-obj-or-name)))
                        (display "Evaluated to: ")
                        (display val)
                        (newline)
                        val))
                    shader-obj-or-name)))
    
    (display "Shader type check: ")
    (display (shader? shader))
    (newline)
    
    (if (shader? shader)
        (begin
          (display "Shader: ")
          (display (shader-name shader))
          (newline)
          
          (display "  Inputs:\n")
          (for-each (lambda (input)
                      (display "    ")
                      (display (car input))
                      (display " (")
                      (display (cadr input))
                      (display ") = ")
                      (display (caddr input))
                      (newline))
                    (shader-inputs shader))
          
          (display "  Uniforms:\n")
          (for-each (lambda (uniform)
                      (display "    ")
                      (display (car uniform))
                      (display " (")
                      (display (cadr uniform))
                      (display ")")
                      (newline))
                    (shader-uniforms shader))
          
          (display "  Varyings:\n")
          (for-each (lambda (varying)
                      (display "    ")
                      (display (car varying))
                      (display " (")
                      (display (cadr varying))
                      (display ")")
                      (newline))
                    (shader-varyings shader))
          
          (display "  Has body function: ")
          (display (if (procedure? (shader-body shader)) "yes" "no"))
          (newline))
        
        (begin
          (display "Not a valid shader object")
          (newline)))))
