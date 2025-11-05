#pragma once

#include <stb_image.h>
#include <tinyxml2.h>
#include "EasyCamera.hpp"
#include "EasyShader.hpp"



class Material {
private:
	struct MaterialLoadInfo
	{
		GLenum internalFormat;
		GLenum format;
		int desiredChannels;
	};

public:
	friend class MaterialLoader;

	enum TextureTypes {
		ALBEDO = 0u,
		AO = 1u,
		METALLIC = 2u,
		ROUGHNESS = 3u,
		NORMAL = 4u,
		HEIGHT = 5u,
		EMISSIVE = 6u,
		OPACITY = 7u,
		UNUSED1 = 8u,
		UNUSED2 = 9u,
		MAX = 10u
	};

	float aoFloating = 0.0f;
	float metallicFloating = 0.0f;
	float roughnessFloating = 0.0f;

	std::string name;

	GLuint GetTexture(TextureTypes type) const
	{
		return textures[type];
	}

	GLuint textures[TextureTypes::MAX] = { (GLuint)0 };

private:
#if defined(CHANNELBITS4)
	inline const Material::MaterialLoadInfo Material::loadInfo[TextureTypes::MAX] = { {GL_RGB4,GL_RGB,3}, {GL_RED,GL_RED,1}, {GL_RED,GL_RED,1},{GL_RED,GL_RED,1},{GL_RGB4,GL_RGB,3},{GL_RED,GL_RED,1},{GL_RGB4,GL_RGB,3},{GL_RED,GL_RED,1},{GL_RED,GL_RED,1},{GL_RED,GL_RED,1} };
#elif defined(COMPRESSION)
	const Material::MaterialLoadInfo Material::loadInfo[TextureTypes::MAX] = { {GL_COMPRESSED_RGB,GL_RGB,3}, {GL_COMPRESSED_RED,GL_RED,1}, {GL_COMPRESSED_RED,GL_RED,1},{GL_COMPRESSED_RED,GL_RED,1},{GL_COMPRESSED_RGB,GL_RGB,3},{GL_COMPRESSED_RED,GL_RED,1},{GL_COMPRESSED_RGB,GL_RGB,3},{GL_COMPRESSED_RED,GL_RED,1},{GL_COMPRESSED_RED,GL_RED,1},{GL_COMPRESSED_RED,GL_RED,1} };
#else
	const Material::MaterialLoadInfo loadInfo[TextureTypes::MAX] = { {GL_RGB,GL_RGB,3}, {GL_RED,GL_RED,1}, {GL_RED,GL_RED,1},{GL_RED,GL_RED,1},{GL_RGB,GL_RGB,3},{GL_RED,GL_RED,1},{GL_RGB,GL_RGB,3},{GL_RED,GL_RED,1},{GL_RED,GL_RED,1},{GL_RED,GL_RED,1} };
#endif

public:
	Material(std::string materialName)
	{
		std::string pathToMaterialFolder = "../../res/";
		tinyxml2::XMLDocument* currentXMLDoc = new tinyxml2::XMLDocument;
		currentXMLDoc->LoadFile(std::string(pathToMaterialFolder + materialName + ".xml").c_str());
		tinyxml2::XMLElement* pbr = currentXMLDoc->FirstChildElement("pbr");

		aoFloating = (float)atof(pbr->FirstChildElement("aoFloating")->GetText());
		metallicFloating = (float)atof(pbr->FirstChildElement("metallicFloating")->GetText());
		roughnessFloating = (float)atof(pbr->FirstChildElement("roughnessFloating")->GetText());

		std::string albedo = pbr->FirstChildElement("albedoTexture")->GetText();
		if (albedo.find("None") == std::string::npos)
			textures[ALBEDO] = LoadImage(pathToMaterialFolder + albedo, loadInfo[ALBEDO].internalFormat, loadInfo[ALBEDO].format, loadInfo[ALBEDO].desiredChannels);

		std::string ao = pbr->FirstChildElement("aoTexture")->GetText();
		if (ao.find("None") == std::string::npos)
			textures[AO] = LoadImage(pathToMaterialFolder + ao, loadInfo[AO].internalFormat, loadInfo[AO].format, loadInfo[AO].desiredChannels);

		std::string metallic = pbr->FirstChildElement("metallicTexture")->GetText();
		if (metallic.find("None") == std::string::npos)
			textures[METALLIC] = LoadImage(pathToMaterialFolder + metallic, loadInfo[METALLIC].internalFormat, loadInfo[METALLIC].format, loadInfo[METALLIC].desiredChannels);

		std::string roughness = pbr->FirstChildElement("roughnessTexture")->GetText();
		if (roughness.find("None") == std::string::npos)
			textures[ROUGHNESS] = LoadImage(pathToMaterialFolder + roughness, loadInfo[ROUGHNESS].internalFormat, loadInfo[ROUGHNESS].format, loadInfo[ROUGHNESS].desiredChannels);

		std::string normal = pbr->FirstChildElement("normalTexture")->GetText();
		if (normal.find("None") == std::string::npos)
			textures[NORMAL] = LoadImage(pathToMaterialFolder + normal, loadInfo[NORMAL].internalFormat, loadInfo[NORMAL].format, loadInfo[NORMAL].desiredChannels);

		if (tinyxml2::XMLElement* elem = pbr->FirstChildElement("emissiveTexture"); elem != nullptr)
		{
			std::string emissive = elem->GetText();
			if (emissive.find("None") == std::string::npos)
				textures[EMISSIVE] = LoadImage(pathToMaterialFolder + emissive, loadInfo[EMISSIVE].internalFormat, loadInfo[EMISSIVE].format, loadInfo[EMISSIVE].desiredChannels);
		}

		if (tinyxml2::XMLElement* elem = pbr->FirstChildElement("opacityTexture"); elem != nullptr)
		{
			std::string opacity = elem->GetText();
			if (opacity.find("None") == std::string::npos)
				textures[OPACITY] = LoadImage(pathToMaterialFolder + opacity, loadInfo[OPACITY].internalFormat, loadInfo[OPACITY].format, loadInfo[OPACITY].desiredChannels);
		}
	}

	static GLuint LoadImage(std::string path, GLenum internalFormat, GLenum format, int desiredChannels)
	{
		GLuint texture = 0;
		int width, height, channels;
		stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, desiredChannels);
		if (data != nullptr)
		{
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);

			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);

			stbi_image_free(data);

			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			assert(false);
		}
		return texture;
	}

};

#define CHUNK_SIZE 250.0f
#define CHUNK_DETAIL 0.50f
constexpr unsigned int CHUNK_STEPS = ((unsigned int)(CHUNK_SIZE / CHUNK_DETAIL));

class HDR;
class Chunk {
public:
	inline static GLuint chunkVAO;
	inline static GLuint chunkVertexSize;

	glm::tvec2<int> gridPos;
	glm::vec4 tilings;

	float* heights;
	float maxHeight;
	float minHeight;

	GLuint blendMap;
	GLuint heightMap;

	Material* backMaterial;
	Material* rMaterial;
	Material* gMaterial;
	Material* bMaterial;

	Chunk(Material* backMaterial, Material* rMaterial, Material* gMaterial, Material* bMaterial, char* blendMapPtr, size_t blendMapLength, char* heightMapPtr, size_t heightMapLength) : backMaterial(backMaterial), rMaterial(rMaterial), gMaterial(gMaterial), bMaterial(bMaterial)
	{
		// Initialize VAO if not Initialized
		if (chunkVAO == 0)
			GenerateChunk();

		// Load Grid
		this->gridPos.x = 0;
		this->gridPos.y = 0;

		// Load Tilings
		this->tilings = glm::vec4(1.f, 1.f, 1.f, 1.f);

		// Craete HeightMap
		glGenTextures(1, &this->heightMap);
		glBindTexture(GL_TEXTURE_2D, this->heightMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, CHUNK_STEPS, CHUNK_STEPS, 0, GL_RED, GL_FLOAT, nullptr);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Load HeightMap
		this->heights = (float*)malloc(CHUNK_STEPS * CHUNK_STEPS * sizeof(float));
		int width, height, channels;
		stbi_uc* heightData = stbi_load_from_memory((stbi_uc*)heightMapPtr, (int)heightMapLength, &width, &height, &channels, 4);
		GenerateHeightMap(heightData);
		stbi_image_free(heightData);

		// Create BlendMap
		glGenTextures(1, &this->blendMap);
		glBindTexture(GL_TEXTURE_2D, this->blendMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, CHUNK_STEPS, CHUNK_STEPS, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Load BlendMap Data
		int blendMapWidth, blendMapHeight, blendMapChannels;
		stbi_uc* data = stbi_load_from_memory((stbi_uc*)blendMapPtr, (int)blendMapLength, &blendMapWidth, &blendMapHeight, &blendMapChannels, 4);
		GenerateBlendMap(data);
		stbi_image_free(data);
	}

	void GenerateBlendMap(stbi_uc* blendData)
	{
		glBindTexture(GL_TEXTURE_2D, this->blendMap);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CHUNK_STEPS, CHUNK_STEPS, GL_RGBA, GL_UNSIGNED_BYTE, blendData);
	}

	void GenerateHeightMap(stbi_uc* heightData) 
	{
		memcpy(this->heights, heightData, CHUNK_STEPS * CHUNK_STEPS * sizeof(float));
		GenerateHeightMapFromTexture((GLuint)0);
	}

	void GenerateHeightMapFromTexture(GLuint fromTexture)
	{
		if (fromTexture != (GLuint)0)
		{
			glBindTexture(GL_TEXTURE_2D, fromTexture);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, this->heights);
		}

		glBindTexture(GL_TEXTURE_2D, this->heightMap);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, CHUNK_STEPS, CHUNK_STEPS, GL_RED, GL_FLOAT, this->heights);

		// Calculate aaBB
		maxHeight = -9999;
		minHeight = 9999;

		float h;
		for (size_t i = 0; i < CHUNK_STEPS * CHUNK_STEPS; i++)
		{
			h = heights[i];
			if (h > maxHeight)
				maxHeight = h;
			if (h < minHeight)
				minHeight = h;
		}
	}

	void GenerateChunk()
	{
		if (chunkVAO != 0)
			return;

		GLuint count = (GLuint)(CHUNK_STEPS)+1;

		std::vector<float> vs;
		vs.resize(count * count * 3);
		GLuint vC = 0;
		for (GLuint z = 0; z < count; z++)
		{
			for (GLuint x = 0; x < count; x++)
			{
				float xx = x * CHUNK_DETAIL - CHUNK_SIZE / 2.0f;
				float yy = 0.0f;
				float zz = z * CHUNK_DETAIL - CHUNK_SIZE / 2.0f;
				vs[vC++] = xx;
				vs[vC++] = yy;
				vs[vC++] = zz;
			}
		}

		std::vector<GLuint> is;
		is.resize(3 * 2 * count * count);
		GLuint iC = 0;
		for (GLuint z = 0; z < count - 1; z++)
		{
			for (GLuint x = 0; x < count - 1; x++)
			{
				GLuint topLeftIndex = x + (z + 1) * count;
				GLuint botLeftIndex = x + z * count;
				GLuint topRightIndex = topLeftIndex + 1;
				GLuint botRightIndex = botLeftIndex + 1;

				is[iC++] = topLeftIndex;
				is[iC++] = botLeftIndex;
				is[iC++] = botRightIndex;

				is[iC++] = botRightIndex;
				is[iC++] = topRightIndex;
				is[iC++] = topLeftIndex;
			}
		}

		chunkVertexSize = (GLuint)is.size();

		GLuint chunkVBO;
		GLuint chunkEBO;
		glGenVertexArrays(1, &chunkVAO);
		glGenBuffers(1, &chunkVBO);
		glGenBuffers(1, &chunkEBO);

		glBindVertexArray(chunkVAO);
		glBindBuffer(GL_ARRAY_BUFFER, chunkVBO);
		glBufferData(GL_ARRAY_BUFFER, vs.size() * sizeof(float), &vs[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, chunkEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, is.size() * sizeof(unsigned int), &is[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		glBindVertexArray(0);
	}
};

class ChunkRenderer {
private:
	static EasyShader* chunkShader;

public:
	static void Init();

	static void Render(EasyCamera* camera, std::vector<Chunk*> chunks, HDR* hdr);

};

