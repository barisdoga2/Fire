#version 400 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;
layout (location = 5) in ivec4 boneIds;
layout (location = 6) in vec4 weights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform mat4 boneMatrices[200];
uniform int animated;

out vec2 pass_uv;

void main()
{
	pass_uv = uv + vec2(normal.x, tangent.y + bitangent.z) * 0.000025;
	vec4 totalPosition = vec4(0.0f);
	if(animated == 1)
	{
		for (int i = 0; i < 4; i++)
		{
			if (boneIds[i] == -1)
				continue; 
			if (boneIds[i] >= 200)
			{
				totalPosition = vec4(position, 1.0f);
				break; 
			}
			vec4 localPosition = boneMatrices[boneIds[i]] * vec4(position, 1.0f); 
			totalPosition += localPosition * weights[i]; 
		}
	}
	else
	{
		totalPosition = vec4(position, 1.0);
	}
	gl_Position = proj * view * model * totalPosition;
}
