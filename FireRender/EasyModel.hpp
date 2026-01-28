#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>

#include <glad/glad.h>

#include "EasyRender.hpp"
#include "EasyUtils.hpp"



struct aiScene;
struct aiMesh;
struct aiNode;
class EasyTexture;
class EasyAnimation;
class EasyAnimator;


class EasyModel {
public:
    struct EasyMesh {
        std::vector<GLuint> indices{};
        std::vector<EasyVertex> vertices{};
        GLuint texture{};
        std::vector<EasyTexture*> textures{};
        GLuint vao{}, vbo{}, ebo{};
        bool animatable{};
        std::string name{};

        bool LoadToGPU();
    };

    std::atomic<bool> isRawDataLoaded{};
    bool isRawDataLoadedToGPU{};

    std::map<std::string, EasyBoneInfo> m_BoneInfoMap;
    std::vector<EasyAnimation*> animations;
    int m_BoneCounter = 0;

    std::unordered_map<EasyMesh*, std::vector<EasyTransform*>> instances;
    bool LoadToGPU();

    int GetBoneID(const std::string& name) const;

    static EasyModel* LoadModel(const std::string& file, const std::vector<std::string> animFiles = {}, glm::vec3 scale = glm::vec3(1,1,1));

private:
    EasyModel();

    static void ProcessMeshes(EasyModel* model, const aiNode* node, const aiScene* scene);
    static void ProcessNodes(EasyModel* model, const aiNode* node, const aiScene* scene);
    static void ExtractBoneWeightForVertices(EasyModel* model, aiMesh* aiMesh, EasyMesh* mesh, const aiScene* scene);
};

