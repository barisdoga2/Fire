#include "EasyMaterial.hpp"
#include "ChunkRenderer.hpp"
#include "HDR.hpp"

#include <glm/gtx/matrix_decompose.hpp>

Chunk::Chunk()
{
    backMaterial = new EasyMaterial("terrainGrass");
}

Chunk::~Chunk()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    VAO = VBO = EBO = 0;
}

float Chunk::FractalNoise2D(float x, float z, float scale, int octaves, float persistence, int seed)
{
    float total = 0.0f;
    float freq = scale;
    float amp = 1.0f;
    float maxAmp = 0.0f;

    for (int i = 0; i < octaves; ++i)
    {
        total += glm::perlin(glm::vec2(x, z) * freq + glm::vec2((float)seed)) * amp;
        maxAmp += amp;
        amp *= persistence;
        freq *= 2.0f;
    }
    return total / maxAmp;
}

void Chunk::LoadToGPU()
{
    if (VAO) glDeleteVertexArrays(1, &VAO);
    if (VBO) glDeleteBuffers(1, &VBO);
    if (EBO) glDeleteBuffers(1, &EBO);
    VAO = VBO = EBO = 0;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 verts.size() * sizeof(ChunkVertex),
                 verts.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 indices.size() * sizeof(unsigned int),
                 indices.data(),
                 GL_STATIC_DRAW);

    // pos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(ChunkVertex),
                          (void*)offsetof(ChunkVertex, position));

    // uv
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(ChunkVertex),
                          (void*)offsetof(ChunkVertex, uv));

    // normal
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
                          sizeof(ChunkVertex),
                          (void*)offsetof(ChunkVertex, normal));

    // tangent
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE,
                          sizeof(ChunkVertex),
                          (void*)offsetof(ChunkVertex, tangent));

    // bitangent (istersen şimdilik kullanmayabilirsin,
    // ama veri hazır dursun)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE,
                          sizeof(ChunkVertex),
                          (void*)offsetof(ChunkVertex, bitangent));

    glBindVertexArray(0);
}

void Chunk::AddTriangle(const glm::vec3& p0, const glm::vec2& uv0,
                        const glm::vec3& p1, const glm::vec2& uv1,
                        const glm::vec3& p2, const glm::vec2& uv2)
{
    glm::vec3 e1 = p1 - p0;
    glm::vec3 e2 = p2 - p0;
    glm::vec2 duv1 = uv1 - uv0;
    glm::vec2 duv2 = uv2 - uv0;

    glm::vec3 normal = glm::normalize(glm::cross(e2, e1));

    float denom = duv1.x * duv2.y - duv1.y * duv2.x;
    glm::vec3 tangent(1.0f, 0.0f, 0.0f);
    glm::vec3 bitangent(0.0f, 0.0f, 1.0f);

    if (fabsf(denom) > 1e-6f)
    {
        float f = 1.0f / denom;
        tangent = f * (duv2.y * e1 - duv1.y * e2);
        bitangent = f * (-duv2.x * e1 + duv1.x * e2);

        tangent = glm::normalize(tangent);
        bitangent = glm::normalize(bitangent);
    }

    ChunkVertex v0, v1, v2;

    v0.position  = p0;
    v0.uv        = uv0;
    v0.normal    = normal;
    v0.tangent   = tangent;
    v0.bitangent = bitangent;

    v1.position  = p1;
    v1.uv        = uv1;
    v1.normal    = normal;
    v1.tangent   = tangent;
    v1.bitangent = bitangent;

    v2.position  = p2;
    v2.uv        = uv2;
    v2.normal    = normal;
    v2.tangent   = tangent;
    v2.bitangent = bitangent;

    unsigned int baseIndex = static_cast<unsigned int>(verts.size());

    verts.push_back(v0);
    verts.push_back(v1);
    verts.push_back(v2);

    indices.push_back(baseIndex + 0);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
}

inline float SmallHeight(float worldX, float worldZ)
{
    float freq = 0.0225f;// 0.6f;      // low frequency → geniş dalgalar
    float amp = 7.25f;// 10.0f;       // +/- 1 metre
    return 0;// glm::perlin(glm::vec2(worldX, worldZ) * freq)* amp;
}

void Chunk::GenerateFlat(const glm::ivec2& inCoord)
{
    coord = inCoord;

    verts.clear();
    indices.clear();

    const int quadsX = CHUNK_STEPS - 1;
    const int quadsZ = CHUNK_STEPS - 1;

    verts.reserve(quadsX * quadsZ * 6);
    indices.reserve(quadsX * quadsZ * 6);

    // chunk world offset
    float worldOffsetX = coord.x * CHUNK_SIZE;
    float worldOffsetZ = coord.y * CHUNK_SIZE;

    for (int z = 0; z < quadsZ; ++z)
    {
        for (int x = 0; x < quadsX; ++x)
        {
            float localX0 = x * DETAIL;
            float localX1 = (x + 1) * DETAIL;
            float localZ0 = z * DETAIL;
            float localZ1 = (z + 1) * DETAIL;

            // world positions for noise continuity
            float wx00 = worldOffsetX + localX0;
            float wz00 = worldOffsetZ + localZ0;
            float wx10 = worldOffsetX + localX1;
            float wz10 = worldOffsetZ + localZ0;
            float wx01 = worldOffsetX + localX0;
            float wz01 = worldOffsetZ + localZ1;
            float wx11 = worldOffsetX + localX1;
            float wz11 = worldOffsetZ + localZ1;

            glm::vec3 p00(localX0 - CHUNK_SIZE / 2.f, SmallHeight(wx00, wz00), localZ0 - CHUNK_SIZE / 2.f);
            glm::vec3 p10(localX1 - CHUNK_SIZE / 2.f, SmallHeight(wx10, wz10), localZ0 - CHUNK_SIZE / 2.f);
            glm::vec3 p01(localX0 - CHUNK_SIZE / 2.f, SmallHeight(wx01, wz01), localZ1 - CHUNK_SIZE / 2.f);
            glm::vec3 p11(localX1 - CHUNK_SIZE / 2.f, SmallHeight(wx11, wz11), localZ1 - CHUNK_SIZE / 2.f);

            float u0 = (float)x / (CHUNK_STEPS - 1);
            float u1 = (float)(x + 1) / (CHUNK_STEPS - 1);
            float v0 = (float)z / (CHUNK_STEPS - 1);
            float v1 = (float)(z + 1) / (CHUNK_STEPS - 1);

            glm::vec2 uv00(u0, v0);
            glm::vec2 uv10(u1, v0);
            glm::vec2 uv01(u0, v1);
            glm::vec2 uv11(u1, v1);

            // Üçgen 1
            AddTriangle(p00, uv00, p10, uv10, p01, uv01);

            // Üçgen 2
            AddTriangle(p10, uv10, p11, uv11, p01, uv01);
        }
    }
}

Chunk* Chunk::GenerateChunkAt(std::vector<Chunk*>& existing, const glm::ivec2& coord, int /*seed*/)
{
    for (auto c : existing)
        if (c->coord == coord)
            return c;

    Chunk* chunk = new Chunk();
    chunk->GenerateFlat(coord);
    existing.push_back(chunk);
    return chunk;
}
void Chunk::AddLake(std::vector<Chunk*>& existing, const glm::vec2& world_coord, float size)
{
    if (existing.empty())
        return;

    const float lakeRadiusX = size * glm::linearRand(0.7f, 1.3f);
    const float lakeRadiusZ = size * glm::linearRand(0.7f, 1.3f);
    const float lakeDepth = size * glm::linearRand(0.08f, 0.15f);
    const float smoothing = glm::linearRand(0.6f, 1.0f);

    for (Chunk* c : existing)
    {
        for (auto& v : c->verts)
        {
            float vx = v.position.x + (c->coord.x * CHUNK_SIZE);
            float vz = v.position.z + (c->coord.y * CHUNK_SIZE);

            float dx = vx - world_coord.x;
            float dz = vz - world_coord.y;
            float nx = dx / lakeRadiusX;
            float nz = dz / lakeRadiusZ;
            float dist = sqrtf(nx * nx + nz * nz);

            if (dist < 1.2f)
            {
                float falloff = 1.0f - glm::smoothstep(0.0f, 1.0f, pow(dist, smoothing));
                float displacement = lakeDepth * falloff;
                v.position.y -= displacement;
                v.position.y += glm::perlin(glm::vec2(vx, vz) * 0.05f) * (lakeDepth * 0.05f);
            }
        }
    }
}




EasyShader* ChunkRenderer::chunkShader;

void ChunkRenderer::Init()
{
	chunkShader = new EasyShader(std::string("Chunk"));
	chunkShader->BindAttribs({ "aPos", "aUV", "aNormal", "aTangent" });
	chunkShader->BindUniforms(GENERAL_UNIFORMS);
	chunkShader->BindUniforms({ "uModel", "uView", "uProj", "uCameraPos"});
	chunkShader->BindTextures({ "diffuseTexture"});
}

void ChunkRenderer::Render(EasyCamera* camera, std::vector<Chunk*> chunks, HDR* hdr, bool fog)
{
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(chunkShader->prog);

	for (Chunk* chunk : chunks)
	{
		glBindVertexArray(chunk->VAO);
		glEnableVertexAttribArray(0);
		chunkShader->LoadUniform("uModel", glm::translate(glm::mat4x4(1), glm::vec3(chunk->coord.x * Chunk::CHUNK_SIZE, 0, chunk->coord.y * Chunk::CHUNK_SIZE)));
		chunkShader->LoadUniform("uView", camera->view_);
		chunkShader->LoadUniform("uProj", camera->projection_);
		chunkShader->LoadUniform("uCameraPos", camera->position);
		chunkShader->LoadUniform("uIsFog", fog ? 1.0f : 0.0f);
        chunkShader->LoadTexture(0, GL_TEXTURE_2D, "diffuseTexture", chunk->backMaterial->GetTexture(EasyMaterial::ALBEDO));

		glDrawElements(GL_TRIANGLES, (GLint)chunk->indices.size(), GL_UNSIGNED_INT, 0);
	}

	glDisableVertexAttribArray(0);
	glBindVertexArray(0);

	glUseProgram(0);
}
