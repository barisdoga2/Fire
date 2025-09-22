#version 400 core

#define MAX_BONES 200
#define MAX_BONE_INFLUENCE 4

// Input variables
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 textureCoords;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 tangent;
layout (location = 4) in ivec4 jointIndices;
layout (location = 5) in vec4 weights;

// Uniform variables
uniform mat4 transformationMatrix;
uniform mat4 meshTransformationMatrix;
uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;
uniform mat4 jointTransforms[MAX_BONES];

// Output Variables
out vec2 pass_textureCoords;

// Function Prototypes
void CalculatePositionAndNormal(inout vec4 totalPosition, inout vec3 totalNormal, inout vec3 totalTangent);

void main()
{
    vec4 totalPosition = vec4(0.0f);
	vec3 totalNormal = vec3(0.0f);
	vec3 totalTangent = vec3(0.0f);

    // Calculate Vertex Position and Normal / Do animations
	CalculatePositionAndNormal(totalPosition, totalNormal, totalTangent);

	// Calculate Actual Vertex Position
	vec4 worldPosition = transformationMatrix * meshTransformationMatrix * totalPosition;
	vec4 positionRelativeToCamera = viewMatrix * worldPosition;
	
    gl_Position = projectionMatrix * positionRelativeToCamera;

	pass_textureCoords = textureCoords;
}

void CalculatePositionAndNormal(inout vec4 totalPosition, inout vec3 totalNormal, inout vec3 totalTangent)
{
	float isAnimated = 1.0;
	if(isAnimated > 0.0)
	{
		for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
		{
			if(jointIndices[i] == -1) 
				continue;
			if(jointIndices[i] >= MAX_BONES) 
			{
				totalPosition = vec4(position,1.0f);
				break;
			}
			vec4 localPosition = jointTransforms[jointIndices[i]] * vec4(position, 1.0f);
			vec3 localNormal = mat3(jointTransforms[jointIndices[i]]) * normal;
			vec3 localTangent = mat3(jointTransforms[jointIndices[i]]) * tangent;
			totalPosition += localPosition * weights[i];
			totalNormal += localNormal;
			totalTangent += localTangent;
		}
	}
	else
	{
		totalPosition = vec4(position, 1);
		totalNormal = normal;
		totalTangent = tangent;
	}
}