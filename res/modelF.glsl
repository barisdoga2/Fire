#version 400 core

in vec2 pass_uv;

uniform sampler2D diffuse;

layout (location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(texture(diffuse, pass_uv).rgb, 1.0);
}
