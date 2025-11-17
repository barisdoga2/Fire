#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out VS_OUT {
    vec3 worldPos;
    vec3 worldNormal;
} vs_out;

void main()
{
    vec4 wp = model * vec4(aPos, 1.0);
    vs_out.worldPos    = wp.xyz;
    vs_out.worldNormal = mat3(transpose(inverse(model))) * aNormal;

    gl_Position = proj * view * wp;
}
