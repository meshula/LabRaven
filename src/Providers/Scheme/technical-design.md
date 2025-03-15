# Technical Design Document: Scheme Shader Transpiler

## Current Status

The Scheme Shader Transpiler is a domain-specific language (DSL) for defining shaders in Scheme that can be transpiled to GLSL and other shader languages. The current implementation allows writing shaders in a natural Scheme syntax, with support for:

- Vector operations and constructors
- Uniform, varying, and input declarations
- Basic BRDF operations
- Multiple target shader languages

The current challenge is handling circular references that occur during shader evaluation and transpilation. In complex shaders like the Disney Principled BRDF, expressions that refer to one another create circular dependencies that must be properly managed.

## A Pragmatic Path Forward

To address the current challenges and enable future expansion, we propose evolving the DSL toward a more structured multi-pass compilation approach.

1. Introduce Special Shader Forms
Replace the generic lambda in shader bodies with domain-specific forms:

```scheme
;; Current approach
(define my-shader
  (make-shader 
    'my-shader
    ;; inputs, uniforms, varyings...
    (lambda ()  ; Generic lambda
      (let* ((normal (normalize 'fragmentNormal))
             ;; ...
             )
        (vec4 results...)))))

;; Proposed approach
(define my-shader
  (make-shader 
    'my-shader
    ;; inputs, uniforms, varyings...
    (fragment-shader ()  ; Special form
      (let* ((normal (normalize 'fragmentNormal))
             ;; ...
             )
        (vec4 results...)))))
```

The fragment-shader form would signal that the body requires special, multi-pass processing rather than direct evaluation.

2. Implement Multi-Pass Processing

Transform the transpilation process into distinct passes:

### Pass 1: Analysis and Symbol Resolution

- Traverse the shader AST without evaluation
- Identify symbols (uniforms, varyings, inputs)
- Detect circular references and mark them for special handling
- Analyze data dependencies and expression complexity

### Pass 2: Variable Declaration Generation

Generate temporary variable declarations for:

- Intermediate expressions in complex calculations
- Expressions involved in circular references
- Expressions used multiple times

### Pass 3: GLSL Code Generation

- Emit variable declarations with appropriate types
- Generate calculation code with proper ordering
- Handle circular references using the temporary variables
- Produce clean, optimized GLSL output

3. Enhanced Symbol Handling

Improve how shader symbols are processed:

```scheme
;; Current approach relies on quoting
(normalize 'fragmentNormal)

;; Proposed approach uses a registry of known symbols
(normalize fragmentNormal)  ;; No quotes needed
```

The shader processor would automatically recognize shader-specific symbols and handle them appropriately, reducing the need for explicit quoting.

4. Circular Reference Resolution

Implement a systematic approach to breaking circular references:

- Identify strongly connected components in the expression dependency graph
- For each component, select a "breaking point" where a temporary variable will be introduced
- Generate appropriate variable declarations and assignments
- Replace circular references with variable references

5. Type System Enhancements

Add a more robust type system to improve GLSL code generation:

- Track expression types through the various operations
- Infer appropriate GLSL types for temporary variables
- Provide better type error messages during shader compilation

## Implementation Plan

### Refactor Shader Representation:

- Define the fragment-shader form to replace lambda
- Modify make-shader to recognize and process the new form

### Implement Analysis Pass:

- Create a non-evaluating AST walker
- Build symbol tables and dependency graphs
- Mark circular references

### Implement Code Generation:

- Generate variable declarations for complex expressions
- Handle expression ordering based on dependencies
- Produce clean GLSL output

### Testing Infrastructure:

- Create test shaders of varying complexity
- Validate GLSL output against expected results
-Test edge cases, especially with circular references

### Documentation and Examples:

- Update documentation to reflect the new approach
- Provide examples of common shader patterns
- Document best practices for shader writing

## Conclusion

By evolving toward a multi-pass compilation approach, the Scheme Shader Transpiler will gain robustness, better handle complex cases like circular references, and provide a foundation for future extensions such as optimization passes, additional target languages, and advanced shader features.
The pragmatic approach outlined here balances the desire to maintain a clean, natural shader writing style with the need to properly handle the complexities that arise in real-world shader development.
