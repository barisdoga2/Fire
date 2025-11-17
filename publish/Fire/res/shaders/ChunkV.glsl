#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aNormal;

out vec2 vUV;
out vec3 vNormal;
out vec3 vWorldPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main()
{
    vUV = aUV;

    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    // No inverse-transpose, no tangent space — basic only
    vNormal = normalize(mat3(uModel) * aNormal);

    gl_Position = uProj * uView * worldPos;
}
