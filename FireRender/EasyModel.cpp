#include "pch.h"
#include "EasyModel.hpp"
#include "EasyTexture.hpp"
#include "EasyAnimation.hpp"
#include "EasyAnimator.hpp"
#include "EasyUtils.hpp"

#include <thread>
#include <iostream>
#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>


bool EasyModel::EasyMesh::LoadToGPU()
{
    if (vao == 0U)
    {
        std::cout << "[EasyMesh] LoadToGPU - Loading mesh '" << name << "'\n";

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(EasyVertex), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(EasyVertex), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(EasyVertex), (void*)offsetof(EasyVertex, uv));

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(EasyVertex), (void*)offsetof(EasyVertex, normal));

        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(EasyVertex), (void*)offsetof(EasyVertex, tangent));

        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(EasyVertex), (void*)offsetof(EasyVertex, bitangent));

        glEnableVertexAttribArray(5);
        glVertexAttribIPointer(5, 4, GL_INT, sizeof(EasyVertex), (void*)offsetof(EasyVertex, boneIds));

        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(EasyVertex), (void*)offsetof(EasyVertex, weights));

        glBindVertexArray(0);
    }

    bool texes = vao;
    for (EasyTexture* tex : textures)
        texes &= tex->LoadToGPU();

    if (texes)
        texture = textures.at(0)->textureID;

    return texes;
}

EasyModel::EasyModel()
{

}

bool EasyModel::LoadToGPU()
{
    bool loaded = true;
    if (isRawDataLoaded.load() && !isRawDataLoadedToGPU)
    {
        for (const auto& kv : instances)
            loaded &= kv.first->LoadToGPU();
        
        if(loaded)
            isRawDataLoadedToGPU = true;
    }
    return isRawDataLoadedToGPU;
}

// STATIC
static inline EasyTexture* NewTexture(std::map<std::string, EasyTexture*>& cache, std::string path, const aiTexture* aiTex, std::vector<EasyTexture*>& out)
{
    std::string name = path;
    name = name.substr(path.find_last_of("/") + 1);

    EasyTexture* tt;
    std::map<std::string, EasyTexture*>::iterator ress = cache.find(name);
    if (ress != cache.end())
    {
        tt = ress->second;
    }
    else
    {
        if (aiTex)
            tt = new EasyTexture(path, aiTex);
        else
            tt = new EasyTexture(path);
        cache.insert({ name, tt });
    }

    auto res = std::find(out.begin(), out.end(), tt);
    if (res == out.end())
        out.push_back(tt);

    return tt;
}

EasyModel* EasyModel::LoadModel(const std::string& file, const std::vector<std::string> animFiles, glm::vec3 scale)
{
    EasyModel* model = new EasyModel();

    std::thread([model, file, animFiles, scale]() {
        std::cout << "[EasyModel] LoadModel - Loading model '" << file << "'\n";
        const aiScene* scene = aiImportFile(file.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_GenBoundingBoxes | aiProcess_CalcTangentSpace);
        assert(scene != nullptr);

        ProcessMeshes(model, scene->mRootNode, scene);
        ProcessNodes(model, scene->mRootNode, scene);

        aiReleaseImport(scene);

        for (const std::string& anfile : animFiles)
        {
            std::cout << "[EasyModel] LoadModel - Loading animation '" << anfile << "'\n";
            const aiScene* animScene = aiImportFile(anfile.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_GenBoundingBoxes | aiProcess_CalcTangentSpace);
            assert(animScene != nullptr && animScene->mNumAnimations == 1);
            std::string animName = anfile.substr(anfile.find_last_of('/') + 1u);
            animName = animName.substr(0, animName.find_last_of('.'));
            model->animations.push_back(new EasyAnimation(animName, animScene, animScene->mAnimations[0], model->m_BoneInfoMap, model->m_BoneCounter));
            aiReleaseImport(animScene);
        }

        for (const auto& kv : model->instances)
            for (auto& kv2 : kv.second)
                kv2->scale *= scale;

        model->isRawDataLoaded.store(true, std::memory_order_release);
    }).detach();

    return model;
}

void EasyModel::ProcessNodes(EasyModel* model, const aiNode* node, const aiScene* scene)
{
    std::string nodeName = node->mName.C_Str();

    for (auto i = 0u; i < node->mNumMeshes; i++)
    {
        std::string aiMeshName = scene->mMeshes[node->mMeshes[i]]->mName.C_Str();
        for (auto& [k, v] : model->instances)
        {
            if (k->name.compare(aiMeshName) == 0)
            {
                v.push_back(new EasyTransformExt(ConvertMatrixToGLMFormat(node->mTransformation)));
                break;
            }
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNodes(model, node->mChildren[i], scene);
    }

    auto boneIt = model->m_BoneInfoMap.find(nodeName);
    if (boneIt != model->m_BoneInfoMap.end())
    {
        int parentID = -1;
        const aiNode* p = node->mParent;
        while (p)
        {
            if (auto it = model->m_BoneInfoMap.find(p->mName.C_Str()); it != model->m_BoneInfoMap.end())
            {
                parentID = it->second.id;
                break;
            }
            p = p->mParent;
        }
        boneIt->second.parent = parentID;
    }
}

void EasyModel::ProcessMeshes(EasyModel* model, const aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* aiMesh = scene->mMeshes[i];
        EasyMesh* easyMesh = new EasyMesh();

        easyMesh->name = aiMesh->mName.C_Str();

        std::cout << "[EasyModel] ProcessMesh - Processing mesh '" << easyMesh->name << "'\n";

        easyMesh->vertices.reserve(aiMesh->mNumVertices);
        for (size_t v = 0; v < aiMesh->mNumVertices; v++)
        {
            EasyVertex ev;
            ev.position = glm::vec3(aiMesh->mVertices[v].x, aiMesh->mVertices[v].y, aiMesh->mVertices[v].z);
            if (aiMesh->HasTextureCoords(0))
                ev.uv = { aiMesh->mTextureCoords[0][v].x, aiMesh->mTextureCoords[0][v].y };
            if (aiMesh->HasNormals())
                ev.normal = { aiMesh->mNormals[v].x, aiMesh->mNormals[v].y, aiMesh->mNormals[v].z };
            if (aiMesh->HasTangentsAndBitangents())
                ev.tangent = { aiMesh->mTangents[v].x, aiMesh->mTangents[v].y, aiMesh->mTangents[v].z };
            if (aiMesh->HasTangentsAndBitangents())
                ev.bitangent = { aiMesh->mBitangents[v].x, aiMesh->mBitangents[v].y, aiMesh->mBitangents[v].z };
            easyMesh->vertices.push_back(ev);
        }

        easyMesh->indices.reserve(aiMesh->mNumFaces * 3);
        for (size_t f = 0; f < aiMesh->mNumFaces; f++)
        {
            easyMesh->indices.push_back((GLuint)aiMesh->mFaces[f].mIndices[0]);
            easyMesh->indices.push_back((GLuint)aiMesh->mFaces[f].mIndices[1]);
            easyMesh->indices.push_back((GLuint)aiMesh->mFaces[f].mIndices[2]);
        }

        ExtractBoneWeightForVertices(model, aiMesh, easyMesh, scene);

        aiString path;
        static std::map<std::string, EasyTexture*> textureCache;
        aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
        {
            if (const aiTexture* embedded = scene->GetEmbeddedTexture(path.C_Str()))
            {
                NewTexture(textureCache, path.C_Str(), embedded, easyMesh->textures);
            }
            else
            {
                NewTexture(textureCache, std::string(GetRelPath("res/images/") + path.C_Str()), nullptr, easyMesh->textures);
            }
        }

        model->instances[easyMesh] = {};
    }
}

void EasyModel::ExtractBoneWeightForVertices(EasyModel* model, aiMesh* aiMesh, EasyMesh* mesh, const aiScene* scene)
{
    aiVertexWeight* weights;
    unsigned int numWeights;
    int vertexId;
    float weight;
    std::string boneName;
    int boneID;
    for (size_t boneIndex = 0; boneIndex < aiMesh->mNumBones; ++boneIndex)
    {
        boneID = -1;
        boneName = aiMesh->mBones[boneIndex]->mName.C_Str();
        if (model->m_BoneInfoMap.find(boneName) == model->m_BoneInfoMap.end())
        {
            model->m_BoneInfoMap[boneName] = { model->m_BoneCounter, -1, AiToGlm(aiMesh->mBones[boneIndex]->mOffsetMatrix) };
            boneID = model->m_BoneCounter;
            model->m_BoneCounter++;
        }
        else
        {
            boneID = model->m_BoneInfoMap[boneName].id;
        }
        assert(boneID != -1);
        weights = aiMesh->mBones[boneIndex]->mWeights;
        numWeights = aiMesh->mBones[boneIndex]->mNumWeights;
        for (unsigned int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
        {
            vertexId = weights[weightIndex].mVertexId;
            weight = weights[weightIndex].mWeight;
            assert(vertexId <= static_cast<int>(mesh->vertices.size()));
            for (int i = 0; i < 4; ++i)
            {
                if (mesh->vertices[vertexId].boneIds[i] < 0)
                {
                    mesh->vertices[vertexId].weights[i] = weight;
                    mesh->vertices[vertexId].boneIds[i] = boneID;
                    break;
                }
            }
        }
    }

    for (auto& v : mesh->vertices)
    {
        float total = v.weights.x + v.weights.y + v.weights.z + v.weights.w;
        if (total > 0.0f)
            v.weights /= total;      // normalize
        for (int i = 0; i < 4; i++)
            if (v.boneIds[i] >= model->m_BoneCounter || v.boneIds[i] < 0)
                v.weights[i] = 0.0f;  // safety
    }

    for (auto idx : mesh->indices)
        assert(idx < mesh->vertices.size());
}

int EasyModel::GetBoneID(const std::string& name) const
{
    const auto& map = animations[0]->BoneInfoMap();
    auto it = map.find(name);
    return it == map.end() ? -1 : it->second.id;
}
