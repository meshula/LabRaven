// Auto-generated GLSL shader from Scheme
precision highp float;

// Uniforms
uniform vec3 lightPosition;
uniform vec3 cameraPosition;

// Varyings
varying vec3 fragmentPosition;
varying vec3 fragmentNormal;

// Shader parameters
const vec3 baseColor = vec3(0.8, 0.8, 0.8);
const float metallic = 0.0;
const float subsurface = 0.0;
const float specular = 0.5;
const float roughness = 0.5;
const float specularTint = 0.0;
const float anisotropic = 0.0;
const float sheen = 0.0;
const float sheenTint = 0.5;
const float clearcoat = 0.0;
const float clearcoatGloss = 1.0;

// BRDF helper functions
float ggxDistribution(float NdotH, float roughness) {
    float alpha2 = roughness * roughness;
    float denom = NdotH * NdotH * (alpha2 - 1.0) + 1.0;
    return alpha2 / (3.14159265 * denom * denom);
}

float smithGGXGeometry(float NdotV, float NdotL, float roughness) {
    float k = roughness * 0.5;
    float ggx1 = NdotV / (NdotV * (1.0 - k) + k);
    float ggx2 = NdotL / (NdotL * (1.0 - k) + k);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(vec3 F0, float HdotV) {
    return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
}

void main() {
    // Normalize vectors
    vec3 normal = normalize(fragmentNormal);
    vec3 viewDir = normalize(cameraPosition - fragmentPosition);
    vec3 lightDir = normalize(lightPosition - fragmentPosition);
    vec3 halfVec = normalize(viewDir + lightDir);
    
    // Calculate dot products
    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.001);
    float NdotH = max(dot(normal, halfVec), 0.0);
    float LdotH = max(dot(lightDir, halfVec), 0.0);
    
    // Calculate specular F0 (mix between dielectric and metallic)
    vec3 F0 = mix(vec3(0.08 * specular), baseColor, metallic);
    
    // Cook-Torrance specular BRDF terms
    float D = ggxDistribution(NdotH, roughness);
    float G = smithGGXGeometry(NdotV, NdotL, roughness);
    vec3 F = fresnelSchlick(F0, LdotH);
    
    // Compute diffuse and specular components
    vec3 diffuse = (1.0 / 3.14159265) * baseColor * (1.0 - metallic);
    vec3 specular = (D * F * G) / (4.0 * NdotV * NdotL);
    
    // Final pixel color (simplified lighting calculation)
    vec3 finalColor = (diffuse + specular) * NdotL;
    
    // Output final color with alpha of 1.0
    gl_FragColor = vec4(finalColor, 1.0);
}
