;; Example of using the Scheme Shader DSL

;; Load the shader system
(load "shader-system.scm")

;; Define a simple phong shader
(define-shader phong-shader
  ;; Inputs
  ((shader-input 'diffuseColor 'vec3 (vec3 0.8 0.2 0.2))
   (shader-input 'specularColor 'vec3 (vec3 1.0 1.0 1.0))
   (shader-input 'shininess 'float 32.0))
  
  ;; Uniforms
  ((uniform 'lightPosition 'vec3)
   (uniform 'cameraPosition 'vec3))
  
  ;; Varyings
  ((varying 'fragmentPosition 'vec3)
   (varying 'fragmentNormal 'vec3))
  
  ;; Shader body - simple Phong lighting model
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
    
    (vec4 final-color 1.0)))

;; Generate GLSL code for the shader
(define phong-glsl (complete-transpile-to-glsl phong-shader))

;; Display the generated GLSL
(display phong-glsl)
(newline)

;; Example of a more complex PBR shader (we already defined disney-principled-brdf earlier)
;; Generate GLSL code for it
(define disney-glsl (complete-transpile-to-glsl disney-principled-brdf))

;; Display the generated GLSL
(display disney-glsl)
(newline)

;; Example of defining a compute shader
(define-shader particle-simulation
  ;; Inputs
  ((shader-input 'deltaTime 'float 0.016)
   (shader-input 'gravity 'vec3 (vec3 0.0 -9.8 0.0)))
  
  ;; Uniforms
  ((uniform 'numParticles 'int))
  
  ;; No varyings for compute shaders, instead we define buffer bindings
  ()
  
  ;; Compute shader body - simplified particle update
  (let* ((particle-id (+ (* gl_GlobalInvocationID.y gl_NumWorkGroups.x gl_WorkGroupSize.x)
                         gl_GlobalInvocationID.x))
         
         ;; Read current particle state
         (position (ssbo-read 'positions particle-id))
         (velocity (ssbo-read 'velocities particle-id))
         
         ;; Update velocity with gravity
         (new-velocity (+ velocity (* gravity deltaTime)))
         
         ;; Update position
         (new-position (+ position (* new-velocity deltaTime))))
    
    ;; Write back to buffers
    (ssbo-write 'positions particle-id new-position)
    (ssbo-write 'velocities particle-id new-velocity)))

;; We would need to extend our transpiler to handle compute shaders,
;; but the concept remains the same - Scheme expressions get translated to GLSL

;; Example of transpiling to multiple shader languages
(define (transpile shader target-language)
  (case target-language
    ((glsl) (complete-transpile-to-glsl shader))
    ((hlsl) (transpile-to-hlsl shader))
    ((msl) (transpile-to-metal-shader-language shader))
    (else (error "Unsupported target language" target-language))))

;; Usage:
;; (transpile disney-principled-brdf 'glsl)
;; (transpile disney-principled-brdf 'hlsl)