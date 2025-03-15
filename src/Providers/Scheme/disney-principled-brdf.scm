;; Scheme Shader DSL for Disney's Principled BRDF
;; Framework for declaring shaders in Scheme and transpiling to GLSL

;; Simple test shader just to verify vec4 functionality
(define test-shader-simple
  (make-shader 
    'test-shader-simple
    (list)  ;; No inputs
    (list)  ;; No uniforms
    (list)  ;; No varyings
    (lambda ()
      ;; Just a simple vec4 construction
      (vec4 0.5 0.6 0.7 1.0))))

;; Test the simplified shader
(display "Testing basic vec4 transpilation:\n")
(let ((glsl-code (complete-transpile-to-glsl test-shader-simple)))
  (display glsl-code)
  (newline))
(display "==================================\n")
(define test-shader-with-helper
  (make-shader 
    'test-shader-with-helper
    ;; Inputs (shader parameters with defaults)
    (list
      (shader-input 'baseColor 'vec3 (vec3 0.8 0.8 0.8))
      (shader-input 'metallic 'float 0.0)
      (shader-input 'subsurface 'float 0.0)
      (shader-input 'specular 'float 0.5)
      (shader-input 'roughness 'float 0.5)
      (shader-input 'specularTint 'float 0.0)
      (shader-input 'anisotropic 'float 0.0)
      (shader-input 'sheen 'float 0.0)
      (shader-input 'sheenTint 'float 0.5)
      (shader-input 'clearcoat 'float 0.0)
      (shader-input 'clearcoatGloss 'float 1.0))

    ;; Uniforms
    (list
      (uniform 'lightPosition 'vec3)
      (uniform 'cameraPosition 'vec3))
    
    ;; Varyings from vertex shader
    (list
      (varying 'fragmentPosition 'vec3)
      (varying 'fragmentNormal 'vec3))

    (lambda ()
      (let* ((normal (normalize 'fragmentNormal))
             (view-dir (normalize (- 'cameraPosition 'fragmentPosition)))
             (light-dir (normalize (- 'lightPosition 'fragmentPosition)))
             (half-vec (normalize (+ view-dir light-dir)))
             (n-dot-l (max (dot normal light-dir) 0.0))
             (n-dot-v (max (dot normal view-dir) 0.001))
             (n-dot-h (max (dot normal half-vec) 0.0))
             (l-dot-h (max (dot light-dir half-vec) 0.0))
            )
        (vec4 n-dot-l n-dot-v n-dot-h 1.0)))))  ;; Use the new helper function

(let ((glsl-code (complete-transpile-to-glsl test-shader-with-helper)))
  (display glsl-code)
  (newline))

(display "==================================\n")

;; Updated Disney Principled BRDF shader using vec4-from-vec3
(define disney-principled-brdf-updated
  (make-shader 
    'disney-principled-brdf-updated
    ;; Inputs (shader parameters with defaults)
    (list
      (shader-input 'baseColor 'vec3 (vec3 0.8 0.8 0.8))
      (shader-input 'metallic 'float 0.0)
      (shader-input 'subsurface 'float 0.0)
      (shader-input 'specular 'float 0.5)
      (shader-input 'roughness 'float 0.5)
      (shader-input 'specularTint 'float 0.0)
      (shader-input 'anisotropic 'float 0.0)
      (shader-input 'sheen 'float 0.0)
      (shader-input 'sheenTint 'float 0.5)
      (shader-input 'clearcoat 'float 0.0)
      (shader-input 'clearcoatGloss 'float 1.0))
    
    ;; Uniforms
    (list
      (uniform 'lightPosition 'vec3)
      (uniform 'cameraPosition 'vec3))
    
    ;; Varyings from vertex shader
    (list
      (varying 'fragmentPosition 'vec3)
      (varying 'fragmentNormal 'vec3))
    
    ;; Shader body (as a lambda)
    (lambda ()
      (let* ((normal (normalize 'fragmentNormal))
             (view-dir (normalize (- 'cameraPosition 'fragmentPosition)))
             (light-dir (normalize (- 'lightPosition 'fragmentPosition)))
             (half-vec (normalize (+ view-dir light-dir)))
             
             (n-dot-l (max (dot normal light-dir) 0.0))
             (n-dot-v (max (dot normal view-dir) 0.001))
             (n-dot-h (max (dot normal half-vec) 0.0))
             (l-dot-h (max (dot light-dir half-vec) 0.0))
             
             ;; Compute specular F0 (mix between dielectric and metallic)
             (f0 (+ (* 'baseColor 'metallic) 
                    (* 0.08 'specular (- 1.0 'metallic))))
             
             ;; Cook-Torrance specular BRDF terms
             (D (ggx-distribution n-dot-h 'roughness))
             (G (smith-ggx-geometry n-dot-v n-dot-l 'roughness))
             (F (fresnel-schlick f0 l-dot-h))
             
             ;; Compute diffuse and specular components
             (diffuse (* (/ 1.0 3.14159265) 'baseColor (- 1.0 'metallic)))
             (specular (/ (* D F G) (* 4.0 n-dot-v n-dot-l)))
             
             ;; Final pixel color (simplified lighting calculation)
             (final-color (* (+ diffuse specular) n-dot-l)))
        
        ;; Return final color with alpha of 1.0
        (vec4-from-vec3 final-color 1.0)))))


;; Test full transpilation
(display "Testing full transpilation of Disney shader:\n")
(let ((glsl-code (complete-transpile-to-glsl disney-principled-brdf-updated)))
  ;; Save the generated shader to a file
  (call-with-output-file "disney_brdf.glsl"
    (lambda (port)
      (display glsl-code port)))
  (display "Shader successfully written to disney_brdf.glsl\n"))
  (display "==================================\n")


;; Define our Disney Principled BRDF fragment shader
(define disney-principled-brdf
  (make-shader 
    'disney-principled-brdf
    ;; Inputs (shader parameters with defaults)
    (list
      (shader-input 'baseColor 'vec3 (vec3 0.8 0.8 0.8))
      (shader-input 'metallic 'float 0.0)
      (shader-input 'subsurface 'float 0.0)
      (shader-input 'specular 'float 0.5)
      (shader-input 'roughness 'float 0.5)
      (shader-input 'specularTint 'float 0.0)
      (shader-input 'anisotropic 'float 0.0)
      (shader-input 'sheen 'float 0.0)
      (shader-input 'sheenTint 'float 0.5)
      (shader-input 'clearcoat 'float 0.0)
      (shader-input 'clearcoatGloss 'float 1.0))
    
    ;; Uniforms
    (list
      (uniform 'lightPosition 'vec3)
      (uniform 'cameraPosition 'vec3))
    
    ;; Varyings from vertex shader
    (list
      (varying 'fragmentPosition 'vec3)
      (varying 'fragmentNormal 'vec3))
    
    ;; Shader body (as a lambda)
    (lambda ()
      (let* ((normal (normalize 'fragmentNormal))
             (view-dir (normalize (- 'cameraPosition 'fragmentPosition)))
             (light-dir (normalize (- 'lightPosition 'fragmentPosition)))
             (half-vec (normalize (+ view-dir light-dir)))
             
             (n-dot-l (max (dot normal light-dir) 0.0))
             (n-dot-v (max (dot normal view-dir) 0.001))
             (n-dot-h (max (dot normal half-vec) 0.0))
             (l-dot-h (max (dot light-dir half-vec) 0.0))
             
             ;; Compute specular F0 (mix between dielectric and metallic)
             (f0 (+ (* 'baseColor 'metallic) 
                    (* 0.08 'specular (- 1.0 'metallic))))
             
             ;; Cook-Torrance specular BRDF terms
             (D (ggx-distribution n-dot-h 'roughness))
             (G (smith-ggx-geometry n-dot-v n-dot-l 'roughness))
             (F (fresnel-schlick f0 l-dot-h))
             
             ;; Compute diffuse and specular components
             (diffuse (* (/ 1.0 3.14159265) 'baseColor (- 1.0 'metallic)))
             (specular (/ (* D F G) (* 4.0 n-dot-v n-dot-l)))
             
             ;; Final pixel color (simplified lighting calculation)
             (final-color (* (+ diffuse specular) n-dot-l)))
        
        ;; Return final color with alpha of 1.0
        (vec4 final-color 1.0)))))

;; Add this at the end of disney-principled-brdf.scm
(display "===== Disney Principled BRDF Shader =====\n")
(describe-shader 'disney-principled-brdf)
(display "=======================================\n")

;; Usage example for GLSL:
;; (complete-transpile 'glsl disney-principled-brdf)

(display "===== Disney BRDF Transpilation =====\n")
(let ((glsl-code (complete-transpile-to-glsl disney-principled-brdf)))
  (display "Transpilation successful!\n")
  (display "First 100 characters of GLSL code:\n")
  (display (substring glsl-code 0 (min 100 (string-length glsl-code))))
  (display "...\n"))
(display "==================================\n")

