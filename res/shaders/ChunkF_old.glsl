#version 400 core

const float GAMMA = 2.2;
const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 7.0;

in vec2 pass_textureCoords;
in vec4 pass_worldPosition;
in vec3 pass_normal;

uniform vec3 cameraPosition;
uniform vec4 tilings;

uniform float hdrRotation;

uniform samplerCube lightingHDRIrradianceMap;
uniform samplerCube lightingHDRPrefilterMap;
uniform sampler2D lightingHDRBrdfLUT;

uniform sampler2D blendMap;
uniform sampler2D heightMap;

uniform sampler2D backAOT;
uniform sampler2D backAlbedoT;
uniform sampler2D backRoughnessT;
uniform sampler2D backNormalT;

uniform sampler2D rAOT;
uniform sampler2D rAlbedoT;
uniform sampler2D rRoughnessT;
uniform sampler2D rNormalT;

uniform sampler2D gAOT;
uniform sampler2D gAlbedoT;
uniform sampler2D gRoughnessT;
uniform sampler2D gNormalT;

uniform sampler2D bAOT;
uniform sampler2D bAlbedoT;
uniform sampler2D bRoughnessT;
uniform sampler2D bNormalT;

layout (location = 0) out vec4 out_color;

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness);
vec3 getNormalFromMap(sampler2D normalT, vec2 textureCoords);

vec3 rotateVector(vec3 v, float angle, vec3 axis) {
    float s = sin(angle);
    float c = cos(angle);
    float oneMinusC = 1.0 - c;
    
    // Rotation matrix
    mat3 rotationMatrix = mat3(
        axis.x * axis.x * oneMinusC + c,
        axis.x * axis.y * oneMinusC - axis.z * s,
        axis.x * axis.z * oneMinusC + axis.y * s,
        
        axis.x * axis.y * oneMinusC + axis.z * s,
        axis.y * axis.y * oneMinusC + c,
        axis.y * axis.z * oneMinusC - axis.x * s,
        
        axis.x * axis.z * oneMinusC - axis.y * s,
        axis.y * axis.z * oneMinusC + axis.x * s,
        axis.z * axis.z * oneMinusC + c
    );
    
    return rotationMatrix * v;
}

void main()
{
	// Get Blend Map Color
	vec3 blendMapColor = texture(blendMap, pass_textureCoords).rgb;

	// Do Tiling
	vec2 backTiledTextureCoords = pass_textureCoords * tilings.r;
	vec2 rTiledTextureCoords = pass_textureCoords * tilings.g;
	vec2 gTiledTextureCoords = pass_textureCoords * tilings.b;
	vec2 bTiledTextureCoords = pass_textureCoords * tilings.a;
	
	// Calculate Factors
	float rTextureFactor = blendMapColor.r;
	float gTextureFactor = blendMapColor.g;
	float bTextureFactor = blendMapColor.b;
    float totalFactor = rTextureFactor + gTextureFactor + bTextureFactor;
    if(totalFactor > 1.0)
    {
        rTextureFactor /= totalFactor;
        gTextureFactor /= totalFactor;
        bTextureFactor /= totalFactor;
    }
	float backTextureFactor = 1.0 - (rTextureFactor + gTextureFactor + bTextureFactor);

	// Calculate Total Albedo
	vec3 backAlbedoC = pow(texture(backAlbedoT, backTiledTextureCoords).rgb, vec3(GAMMA)) * backTextureFactor;
	vec3 rAlbedoC = pow(texture(rAlbedoT, rTiledTextureCoords).rgb, vec3(GAMMA)) * rTextureFactor;
	vec3 gAlbedoC = pow(texture(gAlbedoT, gTiledTextureCoords).rgb, vec3(GAMMA)) * gTextureFactor;
	vec3 bAlbedoC = pow(texture(bAlbedoT, bTiledTextureCoords).rgb, vec3(GAMMA)) * bTextureFactor;
	vec3 totalAlbedoC = backAlbedoC + rAlbedoC + gAlbedoC + bAlbedoC;

	// Calculate Total Ao
	float backAOC = texture(backAOT, backTiledTextureCoords).r * backTextureFactor;
	float rAOC = texture(rAOT, rTiledTextureCoords).r * rTextureFactor;
	float gAOC = texture(gAOT, gTiledTextureCoords).r * gTextureFactor;
	float bAOC = texture(bAOT, bTiledTextureCoords).r * bTextureFactor;
	float totalAOC = backAOC + rAOC + gAOC + bAOC;

	// Calculate Total Roughness
	float backRoughnessC = texture(backRoughnessT, backTiledTextureCoords).r * backTextureFactor;
	float rRoughnessC = texture(rRoughnessT, rTiledTextureCoords).r * rTextureFactor;
	float gRoughnessC = texture(gRoughnessT, gTiledTextureCoords).r * gTextureFactor;
	float bRoughnessC = texture(bRoughnessT, bTiledTextureCoords).r * bTextureFactor;
	float totalRoughnessC = backRoughnessC + rRoughnessC + gRoughnessC + bRoughnessC;

	// Calculate Total Normal
	vec3 backNormalC = getNormalFromMap(backNormalT, backTiledTextureCoords) * backTextureFactor;
	vec3 rNormalC = getNormalFromMap(rNormalT, rTiledTextureCoords) * rTextureFactor;
	vec3 gNormalC = getNormalFromMap(gNormalT, gTiledTextureCoords) * gTextureFactor;
	vec3 bNormalC = getNormalFromMap(bNormalT, bTiledTextureCoords) * bTextureFactor;
	vec3 totalNormalC = backNormalC + rNormalC + gNormalC + bNormalC;

    // Metallic channel is not used for terrain textures, but for paint map
    float totalMetallic = 0.0; 

	// Do PBR Calculation
	vec3 V = normalize(cameraPosition - pass_worldPosition.xyz);
	vec3 R = reflect(-V, totalNormalC); 

	vec3 F0 = vec3(0.04); 
	F0 = mix(F0, totalAlbedoC, totalMetallic);

	vec3 Lo = vec3(0.0);
	vec3 F = fresnelSchlickRoughness(max(dot(totalNormalC, V), 0.0), F0, totalRoughnessC);

	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - totalMetallic;	  

	vec3 rotatedN = rotateVector(totalNormalC, radians(hdrRotation), vec3(0,1,0));
	vec3 rotatedR = rotateVector(R, radians(hdrRotation), vec3(0,1,0));
	vec3 rotatedV = rotateVector(V, radians(hdrRotation), vec3(0,1,0));
	float rotatedNV = max(dot(rotatedN, rotatedV), 0.0);
	
	vec3 irradiance = texture(lightingHDRIrradianceMap, rotatedN).rgb;
	vec3 diffuse      = irradiance * totalAlbedoC;

	vec3 prefilteredColor = textureLod(lightingHDRPrefilterMap, rotatedR,  totalRoughnessC * MAX_REFLECTION_LOD).rgb;    
	vec2 brdf  = texture(lightingHDRBrdfLUT, vec2(rotatedNV, totalRoughnessC)).rg;
	vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	vec3 ambient = (kD * diffuse + specular) * totalAOC;

	vec3 color = ambient + Lo;

	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/GAMMA)); 

	out_color = vec4(color , 1.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

vec3 getNormalFromMap(sampler2D normalT, vec2 textureCoords)
{
    vec3 tangentNormal = texture(normalT, textureCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(pass_worldPosition.xyz);
    vec3 Q2  = dFdy(pass_worldPosition.xyz);
    vec2 st1 = dFdx(textureCoords);
    vec2 st2 = dFdy(textureCoords);

    vec3 N   = normalize(pass_normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}