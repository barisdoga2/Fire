#include "EasyModel.hpp"
#include "EasyTexture.hpp"
#include "EasyAnimation.hpp"
#include "EasyAnimator.hpp"

#include <iostream>
#include <cassert>

#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>



void EasyModel::EasyMesh::LoadToGPU()
{
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

EasyModel::EasyModel(const std::string& file, const std::vector<std::string> animFiles)
{
    const aiScene* scene = aiImportFile(file.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_GenBoundingBoxes | aiProcess_CalcTangentSpace);
    assert(scene != nullptr);

    ProcessNode(scene->mRootNode, scene);

    aiReleaseImport(scene);

    for (std::string file : animFiles)
    {
        const aiScene* animScene = aiImportFile(file.c_str(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_GenBoundingBoxes | aiProcess_CalcTangentSpace);
        assert(animScene != nullptr && animScene->mNumAnimations == 1);
        animations.push_back(new EasyAnimation(animScene, animScene->mAnimations[0], m_BoneInfoMap, m_BoneCounter));
        aiReleaseImport(animScene);
    }
    animator = new EasyAnimator(animations.at(1));
}

bool EasyModel::Update(double _dt, bool mb1_pressed)
{
    EasyAnimation* runAnim = animations.at(1);   // running anim
    EasyAnimation* aimAnim = animations.at(2);   // aiming anim
    animator->UpdateLayered(runAnim, aimAnim, mb1_pressed, _dt);

    return true;
}

void EasyModel::LoadToGPU()
{
    for (auto mesh : meshes)
        mesh->LoadToGPU();
}

EasyModel::EasyMesh* EasyModel::ProcessMesh(aiMesh* aiMesh, const aiScene* scene)
{
    EasyMesh* mesh = new EasyMesh();

    for (size_t v = 0; v < aiMesh->mNumVertices; v++)
    {
        glm::vec3 position = glm::vec3(aiMesh->mVertices[v].x, aiMesh->mVertices[v].y, aiMesh->mVertices[v].z);

        EasyVertex ev;

        ev.position = position;
        if (aiMesh->HasTextureCoords(0))
            ev.uv = { aiMesh->mTextureCoords[0][v].x, aiMesh->mTextureCoords[0][v].y };
        if (aiMesh->HasNormals())
            ev.normal = { aiMesh->mNormals[v].x, aiMesh->mNormals[v].y, aiMesh->mNormals[v].z };
        if (aiMesh->HasTangentsAndBitangents())
            ev.tangent = { aiMesh->mTangents[v].x, aiMesh->mTangents[v].y, aiMesh->mTangents[v].z };
        if (aiMesh->HasTangentsAndBitangents())
            ev.bitangent = { aiMesh->mBitangents[v].x, aiMesh->mBitangents[v].y, aiMesh->mBitangents[v].z };

        mesh->vertices.push_back(ev);
    }

    for (size_t f = 0; f < aiMesh->mNumFaces; f++)
    {
        mesh->indices.push_back((GLuint)aiMesh->mFaces[f].mIndices[0]);
        mesh->indices.push_back((GLuint)aiMesh->mFaces[f].mIndices[1]);
        mesh->indices.push_back((GLuint)aiMesh->mFaces[f].mIndices[2]);
    }

    ExtractBoneWeightForVertices(aiMesh, mesh, scene);

    GLuint texture;
    std::string texPath = "";
    aiString path;
    aiMaterial* material = scene->mMaterials[aiMesh->mMaterialIndex];
    if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
        texPath = std::string(path.C_Str());
    if (const aiTexture* embedded = scene->GetEmbeddedTexture(texPath.c_str()))
        texture = EasyTexture::Load(embedded);
    mesh->texture = texture;

    return mesh;
}

void EasyModel::ProcessNode(const aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(ProcessMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene);
    }
}

void EasyModel::ExtractBoneWeightForVertices(aiMesh* aiMesh, EasyMesh* mesh, const aiScene* scene)
{
    for (size_t boneIndex = 0; boneIndex < aiMesh->mNumBones; ++boneIndex)
    {
        int boneID = -1;
        std::string boneName = aiMesh->mBones[boneIndex]->mName.C_Str();
        if (m_BoneInfoMap.find(boneName) == m_BoneInfoMap.end())
        {
            m_BoneInfoMap[boneName] = { m_BoneCounter, AiToGlm(aiMesh->mBones[boneIndex]->mOffsetMatrix) };
            boneID = m_BoneCounter;
            m_BoneCounter++;
        }
        else
        {
            boneID = m_BoneInfoMap[boneName].id;
        }
        assert(boneID != -1);
        auto weights = aiMesh->mBones[boneIndex]->mWeights;
        int numWeights = aiMesh->mBones[boneIndex]->mNumWeights;
        for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
        {
            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
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
            if (v.boneIds[i] >= m_BoneCounter || v.boneIds[i] < 0)
                v.weights[i] = 0.0f;  // safety
    }

    for (auto idx : mesh->indices)
        assert(idx < mesh->vertices.size());
}
