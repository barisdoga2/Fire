#version 400 core

in vec2 pass_uv;
in vec3 pass_normal;
in vec3 pass_worldPos;

uniform sampler2D diffuse;

out vec4 FragColor;

$$INC$$:Settings.glsl

void main()
{
    // --- Vectors ---
    vec3 N = normalize(pass_normal);
    vec3 L = normalize(-lightDir);
    vec3 V = normalize(uCameraPos - pass_worldPos);
    vec3 H = normalize(L + V);

    // --- Components ---
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), shininess);

    vec3 albedo = texture(diffuse, pass_uv).rgb;

    vec3 ambient  = ambientStrength  * albedo;
    vec3 diffuse  = diffuseStrength  * diff * albedo;
    vec3 specular = specularStrength * spec * lightColor;

    vec3 color = ambient + diffuse + specular;

    if(uIsFog > 0.0)
        FogCalculation(color, pass_worldPos, uCameraPos);

    FragColor = vec4(color, 1.0);
}
