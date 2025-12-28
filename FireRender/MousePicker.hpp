#pragma once

#include <vector>
#include <EasyShader.hpp>


class Chunk;
class EasyShader;
class EasyCamera;
class MousePicker {
public:
	struct PixelInfo {
		GLuint ObjectID = 0;
		GLuint DrawID = 0;
		GLuint PrimID = 0;
		GLuint Type = 0;
	};

	EasyShader* pickShader;

	GLuint fbo;
	GLuint pickTexture;
	GLuint positionTexture;
	GLuint depthTexture;

	glm::vec4 chunkPosition;
	PixelInfo entityInfo;

	MousePicker();

	void Pick(EasyCamera* camera, const std::vector<Chunk*>& chunks);

	PixelInfo ReadPixel(GLuint x, GLuint y);

	glm::vec4 ReadPosition(GLuint x, GLuint y);

};