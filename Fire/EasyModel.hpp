#pragma once

#include <string>
#include <vector>
#include <map>

#include <glad/glad.h>

#include "EasyUtility.hpp"



struct aiScene;
struct aiMesh;
struct aiNode;
class EasyAnimation;
class EasyAnimator;
class EasyModel {
public:
    struct EasyMesh {
        std::vector<GLuint> indices;
        std::vector<EasyVertex> vertices;
        GLuint texture;
        GLuint vao, vbo, ebo;
        void LoadToGPU();
    };

    std::map<std::string, EasyBoneInfo> m_BoneInfoMap;
    std::vector<EasyAnimation*> animations;
    EasyAnimator* animator = nullptr;
    int m_BoneCounter = 0;

    std::vector<EasyMesh*> meshes;

    EasyModel(const std::string& file, const std::vector<std::string> animations = {});
    bool Update(double dt, bool mb1_pressed);
    void LoadToGPU();

private:
    EasyMesh* ProcessMesh(aiMesh* aiMesh, const aiScene* scene);
    void ProcessNode(const aiNode* node, const aiScene* scene);
    void ExtractBoneWeightForVertices(aiMesh* aiMesh, EasyMesh* mesh, const aiScene* scene);
};
