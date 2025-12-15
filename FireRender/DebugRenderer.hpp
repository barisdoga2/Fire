#pragma once

#include <vector>
#include "EasyRender.hpp"

class EasyEntity;
class EasyCamera;
class EasyShader;
class Chunk;
class DebugRenderer {
private:
	static EasyShader* debugShader;

public:
	static void Init(char* debugShaderPtr = nullptr);
	static void DeInit();

	static void Render(EasyCamera* camera, const std::vector<EasyEntity*>& entities, const std::vector<Chunk*>& chunks, const RenderData& rD);

};
