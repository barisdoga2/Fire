// ---- FOG CONSTANTS ----
const float FOG_NEAR = 200.0;
const float FOG_FAR  = 350.0;
const vec3  FOG_COLOR = vec3(0.5, 0.7, 1.0);

// --- BASIC LIGHT ---
vec3 lightDir = normalize(vec3(-0.3, -1.0, -0.4));
vec3 lightColor = vec3(1.0);

// ============================
//   FOG CALCULATION FUNCTION
// ============================
void FogCalculation(in out vec3 color, vec3 worldPos, vec3 cameraPos)
{
    float dist = length(cameraPos - worldPos);
    float fogFactor = clamp((dist - FOG_NEAR) / (FOG_FAR - FOG_NEAR), 0.0, 1.0);
    color = mix(color, FOG_COLOR, fogFactor);
}