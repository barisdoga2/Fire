#pragma once

#include <iostream>
#include <vector>
#include <stb_image.h>
#include <tinyxml2.h>
#include <glm/glm.hpp>
#include <glm/gtc/random.hpp>
#include "EasyCamera.hpp"
#include "EasyShader.hpp"
#include "EasyUtils.hpp"


class HDR;
class EasyMaterial;
class Chunk {
public:
    struct ChunkVertex {
        glm::vec3 position = glm::vec3(0);
        glm::vec2 uv = glm::vec2(0);
        glm::vec3 normal = glm::vec3(0);
        glm::vec3 tangent = glm::vec3(0);
        glm::vec3 bitangent = glm::vec3(0);
    };

    uint8_t id = 0U;
    glm::ivec2 coord = { 0,0 };
    std::vector<ChunkVertex> verts;
    std::vector<unsigned int> indices;

    GLuint VAO = 0, VBO = 0, EBO = 0;

    static constexpr int CHUNK_STEPS = 64;
    static constexpr float CHUNK_SIZE = 256.0f;
    static constexpr float DETAIL = CHUNK_SIZE / (float)(CHUNK_STEPS - 1);
    static constexpr float HEIGHT_SCALE = 32.0f;

    EasyMaterial* backMaterial;

    Chunk();

    ~Chunk();

    void AddTriangle(const glm::vec3& p0, const glm::vec2& uv0,
                        const glm::vec3& p1, const glm::vec2& uv1,
                        const glm::vec3& p2, const glm::vec2& uv2);
    void GenerateFlat(const glm::ivec2& inCoord);
    void LoadToGPU();
    static Chunk* GenerateChunkAt(std::vector<Chunk*>& existing, const glm::ivec2& coord, int seed = 42);
    static void AddLake(std::vector<Chunk*>& existing, const glm::vec2& world_coord, float size);

private:
    static float FractalNoise2D(float x, float z, float scale, int octaves, float persistence, int seed);

};

class ChunkRenderer {
private:
	static EasyShader* chunkShader;

public:
	static void Init();
	static void DeInit();

	static void Render(EasyCamera* camera, std::vector<Chunk*> chunks, HDR* hdr, bool fog);

};

