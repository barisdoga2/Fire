#include "ChunkRenderer.hpp"
#include "HDR.hpp"

#include <glm/gtx/matrix_decompose.hpp>

EasyShader* ChunkRenderer::chunkShader;

void ChunkRenderer::Init()
{
	chunkShader = new EasyShader(std::string("Chunk"));
	chunkShader->BindAttribs({"position"});
	chunkShader->BindUniforms({ "viewMatrix", "projectionMatrix", "transformationMatrix", "cameraPosition", "chunkSize", "tilings", "hdrRotation"});

	chunkShader->BindTextures({ "lightingHDRIrradianceMap", "lightingHDRPrefilterMap", "lightingHDRBrdfLUT"});
	chunkShader->BindTextures({ "blendMap", "heightMap" });
	chunkShader->BindTextures({ "backAOT", "backAlbedoT", "backRoughnessT", "backNormalT" });
	chunkShader->BindTextures({ "rAOT", "rAlbedoT", "rRoughnessT", "rNormalT" });
	chunkShader->BindTextures({ "gAOT", "gAlbedoT", "gRoughnessT", "gNormalT" });
	chunkShader->BindTextures({ "bAOT", "bAlbedoT", "bRoughnessT", "bNormalT" });
}

void ChunkRenderer::Render(EasyCamera* camera, std::vector<Chunk*> chunks, HDR* hdr)
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(chunkShader->prog);

	glBindVertexArray(Chunk::chunkVAO);
	glEnableVertexAttribArray(0);

	chunkShader->LoadUniform("hdrRotation", 0.f);

	int slot = 0;
	chunkShader->LoadTexture(slot++, GL_TEXTURE_CUBE_MAP, "lightingHDRIrradianceMap", hdr->irradianceMap);
	chunkShader->LoadTexture(slot++, GL_TEXTURE_CUBE_MAP, "lightingHDRPrefilterMap", hdr->prefilterMap);
	chunkShader->LoadTexture(slot++, GL_TEXTURE_2D, "lightingHDRBrdfLUT", hdr->brdfLUTTexture);

	chunkShader->LoadUniform("chunkSize", CHUNK_SIZE);
	chunkShader->LoadUniform("cameraPosition", camera->position);
	chunkShader->LoadUniform("viewMatrix", camera->view_);
	chunkShader->LoadUniform("projectionMatrix", camera->projection_);

	for (Chunk* chunk : chunks)
	{
		int slot_ = slot;
		chunkShader->LoadUniform("transformationMatrix", glm::translate(glm::mat4x4(1), glm::vec3(chunk->gridPos.x * CHUNK_SIZE, 0, chunk->gridPos.y * CHUNK_SIZE)));
		chunkShader->LoadUniform("tilings", chunk->tilings);

		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "blendMap", chunk->blendMap);
		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "heightMap", chunk->heightMap);

		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "backAOT", chunk->backMaterial->GetTexture(Material::TextureTypes::AO));
		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "backAlbedoT", chunk->backMaterial->GetTexture(Material::TextureTypes::ALBEDO));
		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "backRoughnessT", chunk->backMaterial->GetTexture(Material::TextureTypes::ROUGHNESS));
		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "backNormalT", chunk->backMaterial->GetTexture(Material::TextureTypes::NORMAL));

		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "rAOT", chunk->rMaterial->GetTexture(Material::TextureTypes::AO));
		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "rAlbedoT", chunk->rMaterial->GetTexture(Material::TextureTypes::ALBEDO));
		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "rRoughnessT", chunk->rMaterial->GetTexture(Material::TextureTypes::ROUGHNESS));
		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "rNormalT", chunk->rMaterial->GetTexture(Material::TextureTypes::NORMAL));

		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "gAOT", chunk->gMaterial->GetTexture(Material::TextureTypes::AO));
		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "gAlbedoT", chunk->gMaterial->GetTexture(Material::TextureTypes::ALBEDO));
		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "gRoughnessT", chunk->gMaterial->GetTexture(Material::TextureTypes::ROUGHNESS));
		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "gNormalT", chunk->gMaterial->GetTexture(Material::TextureTypes::NORMAL));

		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "bAOT", chunk->bMaterial->GetTexture(Material::TextureTypes::AO));
		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "bAlbedoT", chunk->bMaterial->GetTexture(Material::TextureTypes::ALBEDO));
		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "bRoughnessT", chunk->bMaterial->GetTexture(Material::TextureTypes::ROUGHNESS));
		chunkShader->LoadTexture(slot_++, GL_TEXTURE_2D, "bNormalT", chunk->bMaterial->GetTexture(Material::TextureTypes::NORMAL));

		glDrawElements(GL_TRIANGLES, Chunk::chunkVertexSize, GL_UNSIGNED_INT, 0);
	}

	glDisableVertexAttribArray(0);
	glBindVertexArray(0);

	glUseProgram(0);
}
