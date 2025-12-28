#include "MousePicker.hpp"

#include <EasyDisplay.hpp>
#include <EasyCamera.hpp>
#include <EasyEntity.hpp>
#include <ChunkRenderer.hpp>
#include <EasyModel.hpp>

MousePicker::MousePicker()
{
	// Create Shader
	pickShader = new EasyShader(std::string("mousePicker"));
	pickShader->BindAttribs({ "position" });

	pickShader->BindUniforms({ "transformationMatrix","meshTransformationMatrix","projectionMatrix","viewMatrix","gDrawIndex","gObjectIndex","gType" });
	pickShader->BindTextures({ "heightMap" });

	// Create FBO
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glGenTextures(1, &pickTexture);
	glBindTexture(GL_TEXTURE_2D, pickTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32UI, EasyDisplay::GetWindowSize().x, EasyDisplay::GetWindowSize().y, 0, GL_RGBA_INTEGER, GL_UNSIGNED_INT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pickTexture, 0);

	glGenTextures(1, &positionTexture);
	glBindTexture(GL_TEXTURE_2D, positionTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, EasyDisplay::GetWindowSize().x, EasyDisplay::GetWindowSize().y, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, positionTexture, 0);

	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, EasyDisplay::GetWindowSize().x, EasyDisplay::GetWindowSize().y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

	GLenum attachments[2] = { GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, &attachments[0]);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, EasyDisplay::GetWindowSize().x, EasyDisplay::GetWindowSize().y);
}

void MousePicker::Pick(EasyCamera* camera, const std::vector<Chunk*>& chunks)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, EasyDisplay::GetWindowSize().x, EasyDisplay::GetWindowSize().y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	pickShader->Start();

	pickShader->LoadUniform("viewMatrix", camera->ViewMatrix());
	pickShader->LoadUniform("projectionMatrix", camera->ProjectionMatrix());

	// Chunks
	for (Chunk* chunk : chunks)
	{
		glBindVertexArray(chunk->VAO);
		glEnableVertexAttribArray(0);
		pickShader->LoadUniform("meshTransformationMatrix", glm::mat4x4(1));
		pickShader->LoadUniform("gType", (unsigned int)0);
		pickShader->LoadUniform("gObjectIndex", (unsigned int)0);
		{
			// Bind Chunk
			pickShader->LoadTexture(0, GL_TEXTURE_2D, "heightMap", 0);
			pickShader->LoadUniform("gDrawIndex", chunk->id);
			pickShader->LoadUniform("transformationMatrix", glm::translate(glm::mat4x4(1), glm::vec3(chunk->coord.x * Chunk::CHUNK_SIZE, 0, chunk->coord.y * Chunk::CHUNK_SIZE)));

			// Render Chunk
			glDrawElements(GL_TRIANGLES, chunk->verts.size(), GL_UNSIGNED_INT, 0);
		}
		// Unbind Static Chunk
		glDisableVertexAttribArray(0);
		glBindVertexArray(0);
	}

	chunkPosition = ReadPosition((GLuint)EasyMouse::GetCursorPosition().x, (GLuint)EasyDisplay::GetWindowSize().y - (GLuint)EasyMouse::GetCursorPosition().y - 1);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Entities
	//{
	//	pickShader->LoadUniform("gType", (unsigned int)1);
	//	pickShader->LoadUniform("gObjectIndex", (unsigned int)0);
	//	{
	//		for (EasyModel::EasyMesh* mesh : it.first->meshes)
	//		{
	//			//Bind Mesh
	//			glBindVertexArray(mesh->VAO);
	//			glEnableVertexAttribArray(0);
	//			for (EasyEntity* entity : it.second)
	//			{
	//				// Bind EasyEntity
	//				pickShader->LoadUniform("gDrawIndex", entity->id);
	//				pickShader->LoadUniform("transformationMatrix", entity->transformationMatrix);
	//				for (EasyTransformable& instance : mesh->instances)
	//				{
	//					// Render 
	//					pickShader->LoadUniform("meshTransformationMatrix", instance.transformationMatrix);
	//					glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mesh->indices.size()), GL_UNSIGNED_INT, 0);
	//				}
	//			}
	//			// Unbind Mesh
	//			glDisableVertexAttribArray(0);
	//			glBindVertexArray(0);
	//		}
	//	}
	//}

	entityInfo = ReadPixel((GLuint)EasyMouse::GetCursorPosition().x, (GLuint)EasyDisplay::GetWindowSize().y - (GLuint)EasyMouse::GetCursorPosition().y - 1);

	glUseProgram(0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, EasyDisplay::GetWindowSize().x, EasyDisplay::GetWindowSize().y);
}

MousePicker::PixelInfo MousePicker::ReadPixel(GLuint x, GLuint y)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

	glReadBuffer(GL_COLOR_ATTACHMENT0);

	PixelInfo pixel;

	glReadPixels(x, y, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, &pixel);

	glReadBuffer(0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	return pixel;
}

glm::vec4 MousePicker::ReadPosition(GLuint x, GLuint y)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

	glReadBuffer(GL_COLOR_ATTACHMENT1);

	glm::vec4 pixel;

	glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, &pixel);

	glReadBuffer(0);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	return pixel;
}