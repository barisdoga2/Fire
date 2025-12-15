#pragma once

#include <vector>
#include "EasyRenderer.hpp"



class EasyEntity;
class EasyCamera;
class EasyShader;
class ModelRenderer {
private:
	static EasyShader* modelShader;

public:
	static void Init(char* modelShaderPtr = nullptr);
	static void DeInit();

	static void Render(EasyCamera* camera, const std::vector<EasyEntity*>& entities, const RenderData& rD);

};
