# Makefile for shader transpiler integration with S7 Scheme

CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -O2
CXXFLAGS = -Wall -Wextra -O2 -std=c++17
LDFLAGS = -lm -ldl

# S7 paths - adjust these to your system
S7_

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(S7_INCLUDE) -o $@ $^ $(S7_LIB) $(LDFLAGS)

# Create a sample shader definition file
shader-defs.scm:
	@echo "Creating shader definition file..."
	@echo ";; Shader definitions" > $@
	@cat shader-system.scm >> $@
	@cat >> $@ << EOF
;; Define our Disney Principled BRDF fragment shader
(define-shader disney-principled-brdf
  ;; Inputs (shader parameters with defaults)
  ((shader-input 'baseColor 'vec3 (vec3 0.8 0.8 0.8))
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
  ((uniform 'lightPosition 'vec3)
   (uniform 'cameraPosition 'vec3))
  
  ;; Varyings from vertex shader
  ((varying 'fragmentPosition 'vec3)
   (varying 'fragmentNormal 'vec3))
  
  ;; Shader body
  (let* ((normal (normalize fragmentNormal))
         (view-dir (normalize (- cameraPosition fragmentPosition)))
         (light-dir (normalize (- lightPosition fragmentPosition)))
         (half-vec (normalize (+ view-dir light-dir)))
         
         (n-dot-l (max (dot normal light-dir) 0.0))
         (n-dot-v (max (dot normal view-dir) 0.001))
         (n-dot-h (max (dot normal half-vec) 0.0))
         (l-dot-h (max (dot light-dir half-vec) 0.0))
         
         ;; Compute specular F0 (mix between dielectric and metallic)
         (f0 (+ (* baseColor metallic) (* 0.08 specular (- 1.0 metallic))))
         
         ;; Cook-Torrance specular BRDF terms
         (D (ggx-distribution n-dot-h roughness))
         (G (smith-ggx-geometry n-dot-v n-dot-l roughness))
         (F (fresnel-schlick f0 l-dot-h))
         
         ;; Compute diffuse and specular components
         (diffuse (* (/ 1.0 3.14159265) baseColor (- 1.0 metallic)))
         (specular (/ (* D F G) (* 4.0 n-dot-v n-dot-l)))
         
         ;; Final pixel color (simplified lighting calculation)
         (final-color (* (+ diffuse specular) n-dot-l)))
    
    ;; Return final color with alpha of 1.0
    (vec4 final-color 1.0)))
EOF

run: $(TARGET) shader-defs.scm
	./$(TARGET)

clean:
	rm -f $(TARGET) *.o *.glsl

.PHONY: all run clean