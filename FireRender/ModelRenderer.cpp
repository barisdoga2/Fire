#include "pch.h"

#include "ModelRenderer.hpp"
#include <iostream>
#include "EasyShader.hpp"
#include "EasyCamera.hpp"
#include "EasyAnimation.hpp"
#include "EasyAnimator.hpp"

#include "EasyUtils.hpp"
#include "EasyModel.hpp"
#include "GL_Ext.hpp"

EasyShader* ModelRenderer::modelShader = nullptr;

void ModelRenderer::Init(char* modelShaderPtr)
{
	DeInit();

	modelShader = new EasyShader("model");
	modelShader->Start();
	modelShader->BindAttribs({ "position", "uv", "normal", "tangent", "bitangent", "boneIds", "weights" });
	modelShader->BindUniforms(GENERAL_UNIFORMS);
	modelShader->BindUniforms({ "diffuse", "view", "proj", "model", "animated" });
	modelShader->BindUniformArray("boneMatrices", 200);
	modelShader->Stop();
}

void ModelRenderer::DeInit()
{
	if (modelShader)
		delete modelShader;
	modelShader = nullptr;
}

void ModelRenderer::Render(EasyCamera* camera, const std::vector<EasyEntity*>& entities, const RenderData& rD)
{
	modelShader->Start();
	modelShader->LoadUniform("view", camera->view_);
	modelShader->LoadUniform("proj", camera->projection_);
	modelShader->LoadUniform("uCameraPos", camera->position);
	modelShader->LoadUniform("uIsFog", rD.imgui_isFog ? 1.0f : 0.0f);

	for (const EasyEntity* entity : entities)
	{
		if (entity->animator)
			modelShader->LoadUniform("boneMatrices", entity->animator->GetFinalBoneMatrices());

		EasyModel* model = entity->model;

		for (const auto& kv : model->instances)
		{
			EasyModel::EasyMesh* mesh = kv.first;

			if (!mesh->LoadToGPU())
				continue;

			GL(BindVertexArray(mesh->vao));
			GL(EnableVertexAttribArray(0));
			GL(EnableVertexAttribArray(1));
			GL(EnableVertexAttribArray(2));
			GL(EnableVertexAttribArray(3));
			GL(EnableVertexAttribArray(4));
			GL(EnableVertexAttribArray(5));
			GL(EnableVertexAttribArray(6));

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mesh->texture);
			modelShader->LoadUniform("diffuse", 0);

			modelShader->LoadUniform("animated", entity->animator ? 1 : 0);

			for (EasyTransform* t : kv.second)
			{
				modelShader->LoadUniform("model", CreateTransformMatrix(t->position, t->rotationQuat, t->scale));
				glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
			}

			GL(DisableVertexAttribArray(6));
			GL(DisableVertexAttribArray(5));
			GL(DisableVertexAttribArray(4));
			GL(DisableVertexAttribArray(3));
			GL(DisableVertexAttribArray(2));
			GL(DisableVertexAttribArray(1));
			GL(DisableVertexAttribArray(0));
			GL(BindVertexArray(0));
		}
	}

	modelShader->Stop();
}
