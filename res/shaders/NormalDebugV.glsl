#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 vNormalVS;

void main()
{
    mat3 normalMat = mat3(transpose(inverse(model)));
    vNormalVS = normalize(normalMat * aNormal);

    gl_Position = proj * view * model * vec4(aPos, 1.0);
}
