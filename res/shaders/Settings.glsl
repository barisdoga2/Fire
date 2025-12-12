// --- GENERAL UNIFORMS ---
uniform vec3 uCameraPos;
uniform float uIsFog;

// ---- FOG CONSTANTS ----
const float FOG_NEAR = 200.0;
const float FOG_FAR  = 350.0;
const vec3  FOG_COLOR = vec3(0.5, 0.7, 1.0);

// --- BASIC LIGHT ---
const vec3 lightDir = normalize(vec3(-0.3, -1.0, -0.4));
const vec3 lightColor = vec3(1.0);

// --- BLIN PHONG ---
const float ambientStrength  = 0.20;
const float diffuseStrength  = 1.00;
const float specularStrength = 0.20;
const float shininess        = 32.0;

// ============================
//   FOG CALCULATION FUNCTION
// ============================
vec3 FogCalculation(vec3 color, vec3 worldPos, vec3 cameraPos)
{
    float dist = length(cameraPos - worldPos);
    float fogFactor = clamp((dist - FOG_NEAR) / (FOG_FAR - FOG_NEAR), 0.0, 1.0);
    return mix(color, FOG_COLOR, fogFactor);
}