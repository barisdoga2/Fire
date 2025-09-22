#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <assimp/matrix4x4.h>
#include <assimp/vector3.h>
#include <assimp/quaternion.h>
#include <glm/gtc/quaternion.hpp>

static inline float GetCurrentMillis()
{
	static long long unsigned int startTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	long long unsigned int currentTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	return (float)(currentTimestamp - startTimestamp);
}

static inline std::string LoadFile(std::string file)
{
	std::string ret;

	std::string line;
	std::ifstream inFile(file);
	if (inFile.good())
	{
		while (getline(inFile, line))
		{
			if (line.length() > 0)
			{
				ret.append(line + "\n");
			}
		}
		inFile.close();
	}

	return ret;
}

typedef long long unsigned int ts_t;

static inline ts_t GetCurrentNs()
{
	return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

static inline ts_t GetCurrentMs()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

static inline glm::dvec2 ToScreenCoords(glm::dvec2 pos, glm::dvec2 displaySize)
{
	glm::dvec2 retVal;

	retVal.x = pos.x / displaySize.x;
	retVal.y = (displaySize.y - pos.y) / displaySize.y;

	retVal -= 0.5;
	retVal *= 2.0;
	return retVal;
}

static inline glm::vec3 CreateRay(const glm::mat4x4& view, const glm::mat4x4& projection, const glm::vec2& glPos)
{
	glm::vec4 rayEye = glm::inverse(projection) * glm::vec4(glPos.x, glPos.y, 1.f, 1.0f);
	rayEye = glm::vec4(rayEye.x, rayEye.y, -1.f, 0.f);

	glm::mat4x4 invertedView = glm::inverse(view);
	glm::vec4 rayWorld = invertedView * rayEye;
	glm::vec3 mouseRay = glm::vec3(rayWorld.x, rayWorld.y, rayWorld.z);
	mouseRay = glm::normalize(mouseRay);

	return mouseRay;
}


static inline glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
{
	glm::mat4 to;
	//the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
	to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
	to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
	to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
	to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
	return to;
}

static inline glm::vec3 GetGLMVec(const aiVector3D& vec)
{
	return glm::vec3(vec.x, vec.y, vec.z);
}

static inline glm::quat GetGLMQuat(const aiQuaternion& pOrientation)
{
	return glm::quat(pOrientation.w, pOrientation.x, pOrientation.y, pOrientation.z);
}