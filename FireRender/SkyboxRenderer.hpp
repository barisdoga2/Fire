#pragma once

#include "EasyShader.hpp"
#include "EasyCamera.hpp"


class SkyboxRenderer {
private:
	static EasyShader* skyboxShader;
	static GLuint vao;
	static GLuint vbo;
	static size_t vertices;

public:
	static void Init(char* skyboxShaderPtr = nullptr);

	static void Render(EasyCamera& camera);


};
