#include "SkyboxRenderer.hpp"
#include "EasyUtility.hpp"

EasyShader* SkyboxRenderer::skyboxShader = nullptr;
GLuint SkyboxRenderer::vao = 0;
GLuint SkyboxRenderer::vbo = 0;
size_t SkyboxRenderer::vertices = 0;

void SkyboxRenderer::Init(char* skyboxShaderPtr)
{
	if(skyboxShader)
		delete skyboxShader;

	skyboxShader = new EasyShader(std::string("Skybox"));
	skyboxShader->Start();
	skyboxShader->BindAttribs({ "position" });
	skyboxShader->BindUniforms({ "proj", "view" });
	skyboxShader->Stop();

	float Cube3DPositions[108];
	if (vao == 0)
	{
		vao = CreateCube3D(600.0f, &vbo, Cube3DPositions);
		vertices = (size_t)(sizeof(Cube3DPositions) / (sizeof(Cube3DPositions[0]) * 3.0f));
	}
}

void SkyboxRenderer::Render(EasyCamera& camera)
{
	const glm::mat4x4 proj = glm::perspective(glm::radians(camera.fov), camera.aspect, camera.near, 1250.0f);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);

	glUseProgram(skyboxShader->prog);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);

	glm::mat4x4 view = camera.view_;
	view[3][0] = 0;
	view[3][1] = 0;
	view[3][2] = 0;
	skyboxShader->LoadUniform("proj", proj);
	skyboxShader->LoadUniform("view", view);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertices * 3));

	glDisableVertexAttribArray(0);
	glBindVertexArray(0);
	glUseProgram(0);
}
