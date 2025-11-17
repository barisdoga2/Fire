#pragma once

#include <string>
#include <vector>
#include <unordered_map>
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
    class EasyTransform {
    public:
        glm::vec3 position = glm::vec3(0, 0, 0);
        glm::vec3 scale = glm::vec3(1, 1, 1);
        glm::vec3 rotation = glm::vec3(0, 0, 0);
        glm::quat rotationQuat = glm::quat(1, 0, 0, 0);
        EasyTransform(glm::vec3 position, glm::vec3 scale, glm::vec3 rotation) : position(position), scale(scale), rotation(rotation)
        { }
        EasyTransform(glm::vec3 position, glm::vec3 scale, glm::quat rotationQuat) : position(position), scale(scale), rotationQuat(rotationQuat)
        { }
        EasyTransform()
        { }
    };

    struct EasyMesh {
        std::vector<GLuint> indices{};
        std::vector<EasyVertex> vertices{};
        GLuint texture{};
        GLuint vao{}, vbo{}, ebo{};
        bool animatable{};
        std::string name{};

        void LoadToGPU();
    };

    std::map<std::string, EasyBoneInfo> m_BoneInfoMap;
    std::vector<EasyAnimation*> animations;
    EasyAnimator* animator = nullptr;
    int m_BoneCounter = 0;

    std::unordered_map<EasyMesh*, std::vector<EasyTransform*>> instances;

    EasyModel(const std::string& file, const std::vector<std::string> animations = {});
    bool Update(double dt, bool mb1_pressed = false);
    void LoadToGPU();

private:
    EasyMesh* ProcessMesh(aiMesh* aiMesh, const aiScene* scene);
    void ProcessNode(const aiNode* node, const aiScene* scene);
    void ExtractBoneWeightForVertices(aiMesh* aiMesh, EasyMesh* mesh, const aiScene* scene);
};
