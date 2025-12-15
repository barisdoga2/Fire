#pragma once

#include <string>
#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include "EasyShader.hpp"

class HDR {
public:
	static const unsigned int size;
	static const unsigned int irradianceSize;
	static const unsigned int prefilterSize;
	static const unsigned int sampleCount;
	static const unsigned int maxMipLevels;
	static const float sampleDelta;

	std::string name = "";
	GLuint hdrTexture = 0;
	GLuint envCubemap = 0;
	GLuint irradianceMap = 0;
	GLuint prefilterMap = 0;
	GLuint brdfLUTTexture = 0;

	HDR(std::string mapName);
	~HDR();

	static void Init();
	static void DeInit();

private:
	GLuint captureFBO;
	GLuint captureRBO;

	static EasyShader* equirectangularToCubemapShader;
	static EasyShader* irradianceShader;
	static EasyShader* prefilterShader;
	static EasyShader* brdfShader;

	static void RenderQuad();
	static void RenderCube();

};
