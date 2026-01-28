#version 400 core

layout (location = 0) in vec3 position;

out vec3 vPosition;

uniform mat4 proj;
uniform mat4 view;

void main()
{
    vPosition = position;
    gl_Position = proj * view * vec4(position, 1.0);
}
