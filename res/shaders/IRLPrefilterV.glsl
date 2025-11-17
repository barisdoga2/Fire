#version 330 core

layout (location = 0) in vec3 position;

out vec3 pass_worldpos;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

void main()
{
    gl_Position =  projectionMatrix * viewMatrix * vec4(position, 1.0);
    pass_worldpos = position; 
}