#pragma once

#include <string>
#include <glad/glad.h> 

class EasyMaterial {
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

	std::string name;
	float aoFloating = 0.0f;
	float metallicFloating = 0.0f;
	float roughnessFloating = 0.0f;
	GLuint textures[TextureTypes::MAX] = { (GLuint)0 };

	GLuint GetTexture(TextureTypes type) const;

private:
#if defined(CHANNELBITS4)
	inline const EasyMaterial::MaterialLoadInfo EasyMaterial::loadInfo[TextureTypes::MAX] = { {GL_RGB4,GL_RGB,3}, {GL_RED,GL_RED,1}, {GL_RED,GL_RED,1},{GL_RED,GL_RED,1},{GL_RGB4,GL_RGB,3},{GL_RED,GL_RED,1},{GL_RGB4,GL_RGB,3},{GL_RED,GL_RED,1},{GL_RED,GL_RED,1},{GL_RED,GL_RED,1} };
#elif defined(COMPRESSION)
	const EasyMaterial::MaterialLoadInfo EasyMaterial::loadInfo[TextureTypes::MAX] = { {GL_COMPRESSED_RGB,GL_RGB,3}, {GL_COMPRESSED_RED,GL_RED,1}, {GL_COMPRESSED_RED,GL_RED,1},{GL_COMPRESSED_RED,GL_RED,1},{GL_COMPRESSED_RGB,GL_RGB,3},{GL_COMPRESSED_RED,GL_RED,1},{GL_COMPRESSED_RGB,GL_RGB,3},{GL_COMPRESSED_RED,GL_RED,1},{GL_COMPRESSED_RED,GL_RED,1},{GL_COMPRESSED_RED,GL_RED,1} };
#else
	const EasyMaterial::MaterialLoadInfo loadInfo[TextureTypes::MAX] = { {GL_RGB,GL_RGB,3}, {GL_RED,GL_RED,1}, {GL_RED,GL_RED,1},{GL_RED,GL_RED,1},{GL_RGB,GL_RGB,3},{GL_RED,GL_RED,1},{GL_RGB,GL_RGB,3},{GL_RED,GL_RED,1},{GL_RED,GL_RED,1},{GL_RED,GL_RED,1} };
#endif

public:
	EasyMaterial(std::string materialName);

	static GLuint LoadImage(std::string path, GLenum internalFormat, GLenum format, int desiredChannels);

};
