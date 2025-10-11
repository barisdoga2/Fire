#ifdef MAIN
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>

// ---------- Data structures ----------
struct BoneInfo {
    glm::mat4 offset;      // Mesh to bone space
    glm::mat4 transform;   // Final transform
};

struct KeyPosition {
    glm::vec3 position;
    double timeStamp;
};
struct KeyRotation {
    glm::quat orientation;
    double timeStamp;
};
struct KeyScale {
    glm::vec3 scale;
    double timeStamp;
};

struct BoneAnimChannel {
    std::string name;
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale> scales;
};

struct Animation {
    double duration = 0.0;
    double ticksPerSecond = 25.0;
    std::unordered_map<std::string, BoneAnimChannel> channels;
};

struct Bone {
    std::string name;
    glm::mat4 offset;
    std::vector<Bone*> children;
};

struct Skeleton {
    Bone* root = nullptr;
    std::unordered_map<std::string, Bone*> boneMap;
    std::unordered_map<std::string, glm::mat4> finalTransforms;
};

// ---------- Utility conversion ----------
glm::mat4 ConvertMatrix(const aiMatrix4x4& from) {
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

glm::vec3 ConvertVec3(const aiVector3D& v) { return glm::vec3(v.x, v.y, v.z); }
glm::quat ConvertQuat(const aiQuaternion& q) { return glm::quat(q.w, q.x, q.y, q.z); }

// ---------- Recursive skeleton builder ----------
Bone* BuildSkeleton(aiNode* node, const aiScene* scene, Skeleton& skel) {
    Bone* bone = new Bone();
    bone->name = node->mName.C_Str();
    skel.boneMap[bone->name] = bone;

    for (unsigned i = 0; i < node->mNumChildren; ++i) {
        Bone* child = BuildSkeleton(node->mChildren[i], scene, skel);
        bone->children.push_back(child);
    }

    return bone;
}

// ---------- Load mesh and animations ----------
void LoadModel(const std::string& path, Skeleton& skeleton, Animation& animation) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        path,
        aiProcess_Triangulate |
        aiProcess_LimitBoneWeights |
        aiProcess_JoinIdenticalVertices |
        aiProcess_GenSmoothNormals
    );

    if (!scene || !scene->mRootNode)
        throw std::runtime_error("Failed to load model: " + std::string(importer.GetErrorString()));

    std::cout << "Loaded scene: " << path << std::endl;

    // Build skeleton hierarchy
    skeleton.root = BuildSkeleton(scene->mRootNode, scene, skeleton);

    // Collect bone offset matrices
    for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
        aiMesh* mesh = scene->mMeshes[m];
        for (unsigned b = 0; b < mesh->mNumBones; ++b) {
            aiBone* bone = mesh->mBones[b];
            std::string boneName = bone->mName.C_Str();

            glm::mat4 offset = ConvertMatrix(bone->mOffsetMatrix);
            if (skeleton.boneMap.count(boneName)) {
                skeleton.boneMap[boneName]->offset = offset;
            }
        }
    }

    // Load animation
    if (scene->mNumAnimations > 0) {
        aiAnimation* anim = scene->mAnimations[0];
        animation.duration = anim->mDuration;
        animation.ticksPerSecond = anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0;

        for (unsigned c = 0; c < anim->mNumChannels; ++c) {
            aiNodeAnim* chan = anim->mChannels[c];
            BoneAnimChannel channel;
            channel.name = chan->mNodeName.C_Str();

            for (unsigned i = 0; i < chan->mNumPositionKeys; ++i)
                channel.positions.push_back({ ConvertVec3(chan->mPositionKeys[i].mValue), chan->mPositionKeys[i].mTime });

            for (unsigned i = 0; i < chan->mNumRotationKeys; ++i)
                channel.rotations.push_back({ ConvertQuat(chan->mRotationKeys[i].mValue), chan->mRotationKeys[i].mTime });

            for (unsigned i = 0; i < chan->mNumScalingKeys; ++i)
                channel.scales.push_back({ ConvertVec3(chan->mScalingKeys[i].mValue), chan->mScalingKeys[i].mTime });

            animation.channels[channel.name] = std::move(channel);
        }
    }

    std::cout << "Bones loaded: " << skeleton.boneMap.size() << std::endl;
    std::cout << "Animations loaded: " << scene->mNumAnimations << std::endl;
}

int main() {
    Skeleton skeleton;
    Animation animation;

    try {
        LoadModel("../../res/StandingIdle.fbx", skeleton, animation);

        std::cout << "\n=== Skeleton Hierarchy ===\n";
        for (auto& [name, bone] : skeleton.boneMap) {
            std::cout << name << std::endl;
        }

        std::cout << "\n=== Animation Channels ===\n";
        for (auto& [name, channel] : animation.channels) {
            std::cout << "Channel: " << name
                << " PosKeys=" << channel.positions.size()
                << " RotKeys=" << channel.rotations.size()
                << " ScaleKeys=" << channel.scales.size()
                << std::endl;
        }

    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
#endif