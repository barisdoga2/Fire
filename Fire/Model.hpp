#pragma once

#include <vector>
#include <map>
#include <iostream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <ozz/base/maths/soa_float4x4.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/log.h>
#include <ozz/base/io/archive.h>
#include "Utils.hpp"
#include "Bone.hpp"

#define MAX_BONES 200
#define MAX_BONE_INFLUENCE 4

struct BoneInfo
{
	/*id is index in finalBoneMatrices*/
	int id;

	/*offset matrix transforms vertex from model space to bone space*/
	glm::mat4 offset;

};

class Vertex {
public:
	glm::vec3 Position;
	glm::vec2 TexCoords;
	glm::vec3 Normal;
	glm::vec3 Tangent;
	int m_BoneIDs[MAX_BONE_INFLUENCE];
	float m_Weights[MAX_BONE_INFLUENCE];
	glm::vec3 Bitangent;
};

class Transformable {
public:
	glm::mat4x4 transformationMatrix;

	glm::vec3 position = { 0,0,0 };
	glm::vec3 rotation = { 0,0,0 };
	glm::vec3 scale = { 1,1,1 };
};

class Mesh {
public:
	std::vector<Transformable> instances;
	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
	GLuint VAO, VBO, EBO;
	std::string name;
	GLuint albedoT;

	void LoadToGPU()
	{
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));

		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));

		glEnableVertexAttribArray(4);
		glVertexAttribIPointer(4, 4, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));

		glEnableVertexAttribArray(5);
		glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));

		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));

		glBindVertexArray(0);
	}
};

class Animation;
class Model {
public:
	std::string name;
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
	std::vector<Mesh*> meshes;

	// Animations
	std::map<std::string, Animation*> animations;
	std::map<std::string, BoneInfo> m_BoneInfoMap;
	int m_BoneCounter = 0;
	bool animatable;

	// OZZ
	ozz::animation::Skeleton* skeleton;
	std::map<std::string, ozz::animation::Animation*> ozzAnimations;

	Model(std::string name) : name(name)
	{

	}
};

struct AssimpNodeData
{
	glm::mat4 transformation;
	std::string name;
	int childrenCount = 0;
	std::vector<AssimpNodeData> children;
};


class Animation
{
public:
	Model* model;

	Animation() = default;
	std::string name;
	bool cyclic = true;

	Animation(Model* model, const aiScene* scene, std::string name) //+
	{
		this->name = name;
		this->model = model;

		Assimp::Importer importer;
		assert(scene && scene->mRootNode);
		auto animation = scene->mAnimations[0];
		m_Duration = (float)animation->mDuration;
		m_TicksPerSecond = (float)animation->mTicksPerSecond;
		aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
		globalTransformation = globalTransformation.Inverse();
		ReadHeirarchyData(m_RootNode, scene->mRootNode);
		ReadMissingBones(animation, model);
	}

	~Animation()
	{

	}

	Bone* FindBone(const std::string& name) //+
	{
		auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
			[&](const Bone& Bone)
			{
				return Bone.GetBoneName() == name;
			}
		);
		if (iter == m_Bones.end()) return nullptr;
		else return &(*iter);
	}


	inline float GetTicksPerSecond() { return m_TicksPerSecond; }
	inline float GetDuration() { return m_Duration; }
	inline const AssimpNodeData& GetRootNode() { return m_RootNode; }
	inline const std::map<std::string, BoneInfo>& GetBoneIDMap()
	{
		return m_BoneInfoMap;
	}

private:
	void ReadMissingBones(const aiAnimation* animation, Model* model) //+
	{
		int size = animation->mNumChannels;

		auto& boneInfoMap = model->m_BoneInfoMap;//getting m_BoneInfoMap from Model class
		int& boneCount = model->m_BoneCounter; //getting the m_BoneCounter from Model class

		//reading channels(bones engaged in an animation and their keyframes)
		for (int i = 0; i < size; i++)
		{
			auto channel = animation->mChannels[i];
			std::string boneName = channel->mNodeName.data;

			if (boneInfoMap.find(boneName) == boneInfoMap.end())
			{
				boneInfoMap[boneName].id = boneCount;
				boneCount++;
			}
			m_Bones.push_back(Bone(channel->mNodeName.data,
				boneInfoMap[channel->mNodeName.data].id, channel));
		}

		m_BoneInfoMap = boneInfoMap;
	}

	void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src) //+
	{
		assert(src);

		dest.name = src->mName.data;
		dest.transformation = ConvertMatrixToGLMFormat(src->mTransformation);
		dest.childrenCount = src->mNumChildren;

		for (unsigned int i = 0; i < src->mNumChildren; i++)
		{
			AssimpNodeData newData;
			ReadHeirarchyData(newData, src->mChildren[i]);
			dest.children.push_back(newData);
		}
	}

	float m_Duration;
	float m_TicksPerSecond;
	std::vector<Bone> m_Bones;
	AssimpNodeData m_RootNode;
	std::map<std::string, BoneInfo> m_BoneInfoMap;
};



class Animator;
class Entity : public Transformable {
public:
	Model* model;
	Animator* animator;
	Entity(Model* model) : model(model)
	{

	}

	void CreateTransformationMatrix()
	{
		this->transformationMatrix = glm::scale(glm::translate(glm::mat4x4(1), position) * glm::mat4x4(glm::quat(glm::radians(rotation))), scale);
	}
};

void ExtractBoneWeightForVertices(Model* model, std::vector<Vertex>& vertices, aiMesh* mesh, const aiScene* scene)
{
	if (mesh->mNumBones != 0)
		model->animatable = true;

	auto& boneInfoMap = model->m_BoneInfoMap;
	int& boneCount = model->m_BoneCounter;

	for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
	{
		int boneID = -1;
		std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
		if (boneInfoMap.find(boneName) == boneInfoMap.end())
		{
			BoneInfo newBoneInfo;
			newBoneInfo.id = boneCount;
			newBoneInfo.offset = ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
			boneInfoMap[boneName] = newBoneInfo;
			boneID = boneCount;
			boneCount++;
		}
		else
		{
			boneID = boneInfoMap[boneName].id;
		}
		assert(boneID != -1);
		auto weights = mesh->mBones[boneIndex]->mWeights;
		int numWeights = mesh->mBones[boneIndex]->mNumWeights;

		for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
		{
			int vertexId = weights[weightIndex].mVertexId;
			float weight = weights[weightIndex].mWeight;
			assert(vertexId <= (int)vertices.size());

			for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
			{
				if (vertices[vertexId].m_BoneIDs[i] < 0)
				{
					vertices[vertexId].m_Weights[i] = weight;
					vertices[vertexId].m_BoneIDs[i] = boneID;
					break;
				}
			}
		}
	}
}

Mesh* ProcessMesh(Model* model, aiMesh* m, aiNode* node, const aiScene* scene, std::map<aiMesh*, Mesh*>& processedMeshes)
{
	Mesh* mesh = new Mesh();

	unsigned int numUVChannels = m->GetNumUVChannels();

	for (unsigned int i = 0; i < m->mNumVertices; i++)
	{
		Vertex vertex;

		for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
		{
			vertex.m_BoneIDs[i] = -1;
			vertex.m_Weights[i] = 0.0f;
		}

		glm::vec3 vector;
		vector.x = m->mVertices[i].x;
		vector.y = m->mVertices[i].y;
		vector.z = m->mVertices[i].z;
		vertex.Position = vector;

		if (m->HasNormals())
		{
			vector.x = m->mNormals[i].x;
			vector.y = m->mNormals[i].y;
			vector.z = m->mNormals[i].z;
			vertex.Normal = vector;
		}

		// UV Channels + Texture Coords
		{
			if (m->mTextureCoords[0])
			{
				glm::vec2 vec;
				vec.x = m->mTextureCoords[0][i].x;
				vec.y = m->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			//if (m->mTextureCoords[1])
			//{
			//	glm::vec2 vec;
			//	vec.x = m->mTextureCoords[1][i].x;
			//	vec.y = m->mTextureCoords[1][i].y;
			//	vertex.TexCoords2 = vec;
			//}
			//else
			//	vertex.TexCoords2 = glm::vec2(0.0f, 0.0f);
		}

		vector.x = m->mTangents[i].x;
		vector.y = m->mTangents[i].y;
		vector.z = m->mTangents[i].z;
		vertex.Tangent = vector;

		vector.x = m->mBitangents[i].x;
		vector.y = m->mBitangents[i].y;
		vector.z = m->mBitangents[i].z;
		vertex.Bitangent = vector;

		mesh->vertices.push_back(vertex);
	}

	for (unsigned int i = 0; i < m->mNumFaces; i++)
	{
		aiFace face = m->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			mesh->indices.push_back(face.mIndices[j]);
	}

	// Extract BoneIDs and Weights for each vertex
	ExtractBoneWeightForVertices(model, mesh->vertices, m, scene);

	mesh->LoadToGPU();

	// Material
	for (unsigned int mat_idx = 0; mat_idx < scene->mNumMaterials; mat_idx++)
	{
		aiString name;
		aiGetMaterialString(scene->mMaterials[mat_idx], "?mat.name", 0, 0, &name);

		std::cout << name.C_Str() << std::endl;
	}

	aiString materialName;
	aiGetMaterialString(scene->mMaterials[m->mMaterialIndex], "?mat.name", 0, 0, &materialName);

	return mesh;
}

void ProcessNode(Model* rawModel, aiNode* node, const aiScene* scene, std::map<aiMesh*, Mesh*>& processedMeshes)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		std::map<aiMesh*, Mesh*>::iterator res = processedMeshes.find(mesh);
		if (res == processedMeshes.end())
		{
			Mesh* m = ProcessMesh(rawModel, mesh, node, scene, processedMeshes);

			glm::vec3 position;
			glm::vec3 scale;
			glm::quat rotation;
			glm::vec3 skew;
			glm::vec4 perspective;

			Transformable t = Transformable();
			glm::decompose(ConvertMatrixToGLMFormat(node->mTransformation), scale, rotation, position, skew, perspective);
			t.transformationMatrix = glm::scale(glm::translate(glm::mat4x4(1), position) * glm::mat4x4(rotation), scale);

			m->instances.push_back(t);
			rawModel->meshes.push_back(m);
			processedMeshes.insert({ mesh, m });

		}
		else
		{
			glm::vec3 position;
			glm::vec3 scale;
			glm::quat rotation;
			glm::vec3 skew;
			glm::vec4 perspective;

			Transformable t = Transformable();
			glm::decompose(ConvertMatrixToGLMFormat(node->mTransformation), scale, rotation, position, skew, perspective);
			t.transformationMatrix = glm::scale(glm::translate(glm::mat4x4(1), position) * glm::mat4x4(rotation), scale);

			res->second->instances.push_back(t);
		}
	}
	for (unsigned int i = 0; i < node->mNumChildren; i++)
	{
		ProcessNode(rawModel, node->mChildren[i], scene, processedMeshes);
	}

}

bool LoadSkeletonOZZ(const char* _filename, ozz::animation::Skeleton* _skeleton)
{
	assert(_filename && _skeleton);
	ozz::log::Out() << "Loading skeleton archive " << _filename << "."
		<< std::endl;
	ozz::io::File file(_filename, "rb");
	if (!file.opened()) {
		ozz::log::Err() << "Failed to open skeleton file " << _filename << "."
			<< std::endl;
		return false;
	}
	ozz::io::IArchive archive(&file);
	if (!archive.TestTag<ozz::animation::Skeleton>()) {
		ozz::log::Err() << "Failed to load skeleton instance from file "
			<< _filename << "." << std::endl;
		return false;
	}

	// Once the tag is validated, reading cannot fail.
	{
		//ProfileFctLog profile{ "Skeleton loading time" };
		archive >> *_skeleton;
	}
	return true;
}

 bool LoadAnimationOZZ(const char* _filename, ozz::animation::Animation* _animation)
{
	assert(_filename && _animation);
	ozz::log::Out() << "Loading animation archive: " << _filename << "."
		<< std::endl;
	ozz::io::File file(_filename, "rb");
	if (!file.opened()) {
		ozz::log::Err() << "Failed to open animation file " << _filename << "."
			<< std::endl;
		return false;
	}
	ozz::io::IArchive archive(&file);
	if (!archive.TestTag<ozz::animation::Animation>()) {
		ozz::log::Err() << "Failed to load animation instance from file "
			<< _filename << "." << std::endl;
		return false;
	}

	// Once the tag is validated, reading cannot fail.
	{
		//ProfileFctLog profile{ "Animation loading time" };
		archive >> *_animation;
	}

	return true;
}

Model* LoadModel(std::string modelName)
{
	std::string pathToModelFolder = "";

	// Load file
	const aiScene* pScene = aiImportFile(std::string("..\\..\\res\\" + modelName).c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_GenBoundingBoxes | aiProcess_CalcTangentSpace);

	assert(pScene != nullptr);

	Model* model = new Model(modelName);

	glm::quat rotation;
	glm::vec3 skew;
	glm::vec4 perspective;
	glm::decompose(ConvertMatrixToGLMFormat(pScene->mRootNode->mTransformation), model->scale, rotation, model->position, skew, perspective);
	model->rotation = glm::degrees(glm::eulerAngles(rotation));

	std::map<aiMesh*, Mesh*> processedMeshes;
	ProcessNode(model, pScene->mRootNode, pScene, processedMeshes);


	std::vector<std::string> texPaths;
	std::unordered_map<std::string, int> pathToLayer;
	for (unsigned m = 0; m < pScene->mNumMaterials; ++m)
	{
		aiString p;
		if (pScene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, 0, &p) == AI_SUCCESS)
		{
			std::string s = p.C_Str();
			if (!pathToLayer.count(s))
			{
				pathToLayer[s] = texPaths.size();
				texPaths.push_back(s);
			}
		}
	}

	if(false)
	{
		int targetW = 0, targetH = 0;
		struct Img { unsigned char* pixels; int w, h, c; std::string path; };
		std::vector<Img> imgs;
		for (auto& p : texPaths) {
			const aiTexture* emb = pScene->GetEmbeddedTexture(p.c_str());
			int w, h, c;
			unsigned char* pixels = nullptr;
			if (emb)
			{
				if (emb->mHeight == 0) { // compressed (png/jpg) in memory
					int comp;
					unsigned char* img = stbi_load_from_memory((unsigned char*)emb->pcData, emb->mWidth, &w, &h, &comp, 4);
					c = 4;
					pixels = img;
				}
				else { // raw RGBA8888
					w = emb->mWidth; h = emb->mHeight; c = 4;
					unsigned char* img = (unsigned char*)malloc(w * h * 4);
					// aiTexFormat stores RGBA as contiguous bytes
					memcpy(img, emb->pcData, w * h * 4);
					pixels = img;
				}
			}
			else { pixels = stbi_load(p.c_str(), &w, &h, &c, 4); c = 4; }
			if (!pixels) { /* handle missing */ }
			if (targetW == 0) { targetW = w; targetH = h; }
			if (w != targetW || h != targetH) {
				// resize to targetW/targetH (or choose to resize all to first texture)
				unsigned char* tmp = (unsigned char*)malloc(targetW * targetH * 4);
				stbir_resize_uint8_linear(pixels, w, h, 0, tmp, targetW, targetH, 0, STBIR_RGBA);
				stbi_image_free(pixels);
				pixels = tmp; w = targetW; h = targetH;
			}
			imgs.push_back({ pixels,w,h,4,p });
		}

		// Create GL texture2D array
		GLuint texArray; glGenTextures(1, &texArray);
		glBindTexture(GL_TEXTURE_2D_ARRAY, texArray);
		int layers = (int)imgs.size();
		int mipLevels = 1 + (int)floor(log2(targetW > targetH ? targetW : targetH));
		glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevels, GL_RGBA8, targetW, targetH, layers);

		// upload each layer
		for (int layer = 0; layer < layers; ++layer) {
			Img& I = imgs[layer];
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layer, I.w, I.h, 1, GL_RGBA, GL_UNSIGNED_BYTE, I.pixels);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	}
	else
	{
		std::string p = texPaths.at(0);
		int w, h, c;
		unsigned char* pixels = nullptr;

		// check if embedded in FBX
		const aiTexture* emb = pScene->GetEmbeddedTexture(p.c_str());
		if (emb) {
			if (emb->mHeight == 0) { // compressed (png/jpg)
				pixels = stbi_load_from_memory((unsigned char*)emb->pcData, emb->mWidth,
					&w, &h, &c, 4);
			}
			else { // raw BGRA/RGBA
				w = emb->mWidth; h = emb->mHeight; c = 4;
				pixels = (unsigned char*)malloc(w * h * 4);
				memcpy(pixels, emb->pcData, w * h * 4);
			}
		}
		else {
			pixels = stbi_load(p.c_str(), &w, &h, &c, 4);
		}
		if (!pixels) {
			std::cerr << "Failed to load texture: " << p << std::endl;
			return 0;
		}

		GLuint texID;
		glGenTextures(1, &texID);
		glBindTexture(GL_TEXTURE_2D, texID);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindTexture(GL_TEXTURE_2D, 0);

		stbi_image_free(pixels);

		for (Mesh* m : model->meshes)
			m->albedoT = texID;
	}



	aiReleaseImport(pScene);

	std::string animName = std::string("..\\..\\res\\") + modelName.substr(0, modelName.find_last_of(".")) + std::string("_OrcIdle.fbx");
	const aiScene* pAnimScene = aiImportFile(animName.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_GenBoundingBoxes | aiProcess_CalcTangentSpace);

	model->animations["idle"] = new Animation(model, pAnimScene, "idle");

	ozz::animation::Skeleton* skeletonOZZ = new ozz::animation::Skeleton();
	std::string skeNameOzz = std::string(animName.substr(0, animName.find_last_of("_")) + "_skeleton.ozz");
	if (!LoadSkeletonOZZ(skeNameOzz.c_str(), skeletonOZZ))
	{
		std::cout << "OZZ LoadSkeleton error: " << skeNameOzz << std::endl;
		delete skeletonOZZ;
		skeletonOZZ = nullptr;
	}
	else
	{
		model->skeleton = skeletonOZZ;
	}

	ozz::animation::Animation* animationOZZ = new ozz::animation::Animation();
	std::string animNameOzz = std::string(animName.substr(0, animName.find_last_of(".")) + ".ozz");
	if (!LoadAnimationOZZ(animNameOzz.c_str(), animationOZZ))
	{
		std::cout << "OZZ LoadAnimation error: " << animNameOzz << std::endl;
		delete animationOZZ;
		animationOZZ = nullptr;
	}
	else
	{
		model->ozzAnimations.insert({ std::string("idle"), animationOZZ });
	}

	return model;
}
