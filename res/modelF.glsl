#version 420 core

in vec2 pass_textureCoords;

uniform sampler2D albedoT;

layout (location = 0) out vec4 out_color;

void main()
{
	// Albedo
    vec3 albedo = pow(texture(albedoT, pass_textureCoords).rgb, vec3(1.0));

    out_color = vec4(albedo, 1.0);

}
