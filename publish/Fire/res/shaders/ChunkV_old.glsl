#version 400 core

layout (location = 0) in vec3 position;

uniform mat4 transformationMatrix;
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform float chunkSize;

uniform sampler2D blendMap;
uniform sampler2D heightMap;

out vec2 pass_textureCoords;
out vec4 pass_worldPosition;
out vec3 pass_normal;

void main()
{
	pass_textureCoords = vec2((position.x / (chunkSize/2)) / 2.0 + 0.5, ((position.z / (chunkSize/2)) / 2.0 + 0.5));
	float height = texture(heightMap, pass_textureCoords).r;
	pass_worldPosition = transformationMatrix * vec4(vec3(position.x, height, position.z), 1.0);
	
    gl_Position = projectionMatrix * viewMatrix * pass_worldPosition;

	float texel = 1.0 / 500.0;
	vec3 offset = vec3(1,1,0);
	float hL = texture(heightMap, pass_textureCoords - offset.xz * texel).r;
	float hR = texture(heightMap, pass_textureCoords + offset.xz * texel).r;
	float hD = texture(heightMap, pass_textureCoords - offset.zy * texel).r;
	float hU = texture(heightMap, pass_textureCoords + offset.xy * texel).r;

	//pass_normal = normalize(vec3(hL - hR, 1.0, hD - hU));
	pass_normal = normalize(vec3(2*(hL - hR), 4.0, 2*(hD - hU)));

}
