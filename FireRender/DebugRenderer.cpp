#include "pch.h"

#include "DebugRenderer.hpp"
#include <iostream>
#include "EasyShader.hpp"
#include "EasyCamera.hpp"
#include "ChunkRenderer.hpp"

#include "EasyUtils.hpp"
#include "EasyModel.hpp"
#include "GL_Ext.hpp"

EasyShader* DebugRenderer::debugShader = nullptr;

void DebugRenderer::Init(char* modelShaderPtr)
{
	DeInit();

	debugShader = new EasyShader("NormalLines");
	debugShader->Start();
	debugShader->BindAttribs({ "aPos", "aUV", "aNormal" });
	debugShader->BindUniforms({ "model", "view", "proj", "normalLength" });
	debugShader->Stop();
}

void DebugRenderer::DeInit()
{
	if (debugShader)
		delete debugShader;
	debugShader = nullptr;
}

void DebugRenderer::Render(EasyCamera* camera, const std::vector<EasyEntity*>& entities, const std::vector<Chunk*>& chunks, const RenderData& rD)
{
	if (!rD.imgui_showNormalLines)
		return;

	debugShader->Start();

	debugShader->LoadUniform("view", camera->view_);
	debugShader->LoadUniform("proj", camera->projection_);
	debugShader->LoadUniform("normalLength", rD.imgui_showNormalLength);

	// Objects
	for (EasyEntity* entity : entities)
	{
		EasyModel* model = entity->model;
		for (const auto& kv : model->instances)
		{
			EasyModel::EasyMesh* mesh = kv.first;

			if (!mesh->LoadToGPU())
				continue;

			GL(BindVertexArray(mesh->vao));
			GL(EnableVertexAttribArray(0));
			GL(EnableVertexAttribArray(2));

			for (EasyTransform* t : kv.second)
			{
				debugShader->LoadUniform("model", CreateTransformMatrix(t->position, t->rotationQuat, t->scale));

				glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
			}

			GL(DisableVertexAttribArray(2));
			GL(DisableVertexAttribArray(0));
			GL(BindVertexArray(0));
		}
	}

	// Chunks
	for (Chunk* chunk : chunks)
	{
		glBindVertexArray(chunk->VAO);
		glEnableVertexAttribArray(0);
		debugShader->LoadUniform("model", glm::translate(glm::mat4x4(1), glm::vec3(chunk->coord.x * Chunk::CHUNK_SIZE, 0, chunk->coord.y * Chunk::CHUNK_SIZE)));

		glDrawElements(GL_TRIANGLES, (GLint)chunk->indices.size(), GL_UNSIGNED_INT, 0);

		glDisableVertexAttribArray(0);
		glBindVertexArray(0);
	}

	debugShader->Stop();
}
