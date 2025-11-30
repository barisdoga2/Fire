#include "pch.h"
#include <assert.h>
#include <tinyxml2.h>
#include <stb_image.h>
#include "EasyMaterial.hpp"
#include "EasyUtility.hpp"

GLuint EasyMaterial::GetTexture(TextureTypes type) const
{
	return textures[type];
}

EasyMaterial::EasyMaterial(std::string materialName)
{
	std::string pathToMaterialFolder = GetPath("res/materials/");
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

GLuint EasyMaterial::LoadImage(std::string path, GLenum internalFormat, GLenum format, int desiredChannels)
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

