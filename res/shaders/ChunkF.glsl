#version 330 core

in vec2 vUV;
in vec3 vNormal;
in vec3 vWorldPos;

uniform vec3 uCameraPos;
uniform sampler2D diffuseTexture;

out vec4 FragColor;

$$INC$$:Settings.glsl


void main()
{
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-lightDir);
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);

    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 32.0);

    vec3 albedo = texture(diffuseTexture, vUV).rgb;

    vec3 ambient  = albedo * 0.20;
    vec3 diffuseTextureC = albedo * diff;
    vec3 specular = vec3(1.0) * spec * 0.20;

    vec3 color = ambient + diffuseTextureC + specular;

    FogCalculation(color, vWorldPos, uCameraPos);

    FragColor = vec4(color, 1.0);
}
