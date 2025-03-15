/**
 * shader_transpiler.c - Integration with S7 Scheme 
 * 
 * This demonstrates how to use our Scheme shader transpiler
 * from C/C++ using Stanford's S7 Scheme implementation.
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include "s7.h"
 
 // Helper function to load and evaluate a Scheme file
 static s7_pointer load_scheme_file(s7_scheme *s7, const char *filename) {
     FILE *fp = fopen(filename, "r");
     if (!fp) {
         fprintf(stderr, "Error: couldn't open file %s\n", filename);
         return s7_nil(s7);
     }
     
     // Get file size
     fseek(fp, 0, SEEK_END);
     long file_size = ftell(fp);
     fseek(fp, 0, SEEK_SET);
     
     // Read the entire file
     char *buffer = (char*)malloc(file_size + 1);
     size_t bytes_read = fread(buffer, 1, file_size, fp);
     buffer[bytes_read] = '\0';
     fclose(fp);
     
     // Evaluate the content
     s7_pointer result = s7_eval_c_string(s7, buffer);
     free(buffer);
     
     return result;
 }
 
 // Helper to check for Scheme errors
 static void check_for_errors(s7_scheme *s7, s7_pointer result) {
     if (s7_is_error(result)) {
         fprintf(stderr, "Scheme error: %s\n", s7_object_to_c_string(s7, result));
         exit(1);
     }
 }
 
 // Main program
 int main(int argc, char *argv[]) {
     // Initialize S7 Scheme interpreter
     s7_scheme *s7 = s7_init();
     if (!s7) {
         fprintf(stderr, "Failed to initialize S7 Scheme\n");
         return 1;
     }
     
     printf("Initializing Scheme shader transpiler...\n");
     
     // Load our shader system implementation
     s7_pointer result = load_scheme_file(s7, "shader-system.scm");
     check_for_errors(s7, result);
     
     // Load shader definitions including the Disney BRDF
     result = load_scheme_file(s7, "shader-defs.scm");
     check_for_errors(s7, result);
     
     printf("Shader system loaded successfully.\n");
     
     // Get the disney-principled-brdf shader
     s7_pointer shader = s7_name_to_value(s7, "disney-principled-brdf");
     if (s7_is_null(s7, shader)) {
         fprintf(stderr, "Error: disney-principled-brdf not defined\n");
         return 1;
     }
     
     printf("Transpiling Disney Principled BRDF shader to GLSL...\n");
     
     // Call the transpiler function
     s7_pointer transpile_args = s7_list(s7, 1, shader);
     s7_pointer glsl_code = s7_call(s7, s7_name_to_value(s7, "complete-transpile-to-glsl"), transpile_args);
     check_for_errors(s7, glsl_code);
     
     // Convert result to C string
     const char *glsl_str = s7_string(glsl_code);
     
     // Output the GLSL code
     printf("\nGenerated GLSL code:\n");
     printf("===================\n");
     printf("%s\n", glsl_str);
     
     // Save to file
     FILE *output = fopen("disney_brdf.glsl", "w");
     if (output) {
         fprintf(output, "%s", glsl_str);
         fclose(output);
         printf("\nShader successfully written to disney_brdf.glsl\n");
     } else {
         fprintf(stderr, "Error: Could not write shader to file\n");
     }
     
     // Demonstrate modifying shader parameters
     printf("\nModifying shader parameters...\n");
     
     // Set new roughness parameter (0.2 instead of default 0.5)
     result = s7_eval_c_string(s7, 
         "(define modified-shader\n"
         "  (make-shader\n"
         "    (shader-name disney-principled-brdf)\n"
         "    (map (lambda (input)\n"
         "           (if (eq? (car input) 'roughness)\n"
         "               (list 'roughness 'float 0.2)\n"
         "               input))\n"
         "         (shader-inputs disney-principled-brdf))\n"
         "    (shader-uniforms disney-principled-brdf)\n"
         "    (shader-varyings disney-principled-brdf)\n"
         "    (shader-body disney-principled-brdf)))"
     );
     check_for_errors(s7, result);
     
     // Transpile the modified shader
     s7_pointer modified_shader = s7_name_to_value(s7, "modified-shader");
     transpile_args = s7_list(s7, 1, modified_shader);
     glsl_code = s7_call(s7, s7_name_to_value(s7, "complete-transpile-to-glsl"), transpile_args);
     check_for_errors(s7, glsl_code);
     
     // Save the modified shader
     output = fopen("disney_brdf_modified.glsl", "w");
     if (output) {
         fprintf(output, "%s", s7_string(glsl_code));
         fclose(output);
         printf("Modified shader written to disney_brdf_modified.glsl\n");
     }
     
     // Clean up
     s7_quit(s7);
     
     return 0;
 }