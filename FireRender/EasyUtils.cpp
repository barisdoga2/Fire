#include "pch.h"

#include "EasyUtils.hpp"

#include <glad/glad.h>
#include <glm/gtc/noise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma comment(lib, "dbghelp.lib")



// Maths
float Noise2D(float x, float z, float scale, int seed)
{
    return glm::perlin(glm::vec2(x, z) * scale + glm::vec2(seed * 0.1234f));
}

float FractalNoise2D(float x, float z, float scale, int octaves, float persistence, int seed)
{
    float total = 0.0f;
    float freq = scale;
    float amp = 1.0f;
    float maxAmp = 0.0f;

    for (int i = 0; i < octaves; ++i)
    {
        total += glm::perlin(glm::vec2(x, z) * freq + float(seed)) * amp;
        maxAmp += amp;
        amp *= persistence;   // reduces amplitude each octave
        freq *= 2.0f;         // doubles detail each octave
    }

    return total / maxAmp; // normalize
}

glm::mat4 AiToGlm(const aiMatrix4x4& m)
{
    glm::mat4 result{};
    result[0][0] = m.a1; result[1][0] = m.a2; result[2][0] = m.a3; result[3][0] = m.a4;
    result[0][1] = m.b1; result[1][1] = m.b2; result[2][1] = m.b3; result[3][1] = m.b4;
    result[0][2] = m.c1; result[1][2] = m.c2; result[2][2] = m.c3; result[3][2] = m.c4;
    result[0][3] = m.d1; result[1][3] = m.d2; result[2][3] = m.d3; result[3][3] = m.d4;
    return result;
}

glm::vec3 AiToGlm(const aiVector3D& m)
{
    glm::vec3 result{};
    result.x = m.x;
    result.y = m.y;
    result.z = m.z;
    return result;
}

glm::quat AiToGlm(const aiQuaternion& q)
{
    return glm::quat(q.w, q.x, q.y, q.z);
}

glm::mat4 CreateTransformMatrix(const glm::vec3& position, const glm::vec3& rotationDeg, const glm::vec3& scale)
{
    // Convert degrees to radians
    glm::vec3 rotation = glm::radians(rotationDeg);

    // Build individual matrices
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotationMat = glm::yawPitchRoll(rotation.y, rotation.x, rotation.z);
    glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);

    // Combine (T * R * S)
    glm::mat4 transform = translation * rotationMat * scaling;

    return transform;
}

glm::mat4 CreateTransformMatrix(const glm::vec3& position, const glm::quat& rotationQuat, const glm::vec3& scale)
{
    glm::mat4 translation = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotationMat = glm::toMat4(rotationQuat);
    glm::mat4 scaling = glm::scale(glm::mat4(1.0f), scale);

    // Same order as before: T * R * S
    return translation * rotationMat * scaling;
}

glm::mat4 ConvertMatrixToGLMFormat(const aiMatrix4x4& from)
{
    glm::mat4 to{};
    //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}



// OpenGL
unsigned int CreateCube3D(float size, unsigned int* vbo, float* positions_out)
{
    static const float Cube3DPositions[108] = { -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f,-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f };
    static const GLuint vertices = (GLuint)(sizeof(Cube3DPositions) / (sizeof(float) * 3.0f));

    GLuint vao;
    GLuint vbo_;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    float* positions = (float*)malloc((size_t)vertices * 3 * sizeof(float));
    if (positions != nullptr)
    {
        std::memcpy(positions, Cube3DPositions, (size_t)vertices * 3 * sizeof(float));
        for (GLuint i = 0; i < vertices; i++)
        {
            positions[i * 3 + 0] *= size;
            positions[i * 3 + 1] *= size;
            positions[i * 3 + 2] *= size;
        }

        if(positions_out)
            memcpy(positions_out, &positions, (size_t)vertices * 3 * sizeof(float));

        if (vbo == nullptr)
        {
            glGenBuffers(1, &vbo_);
            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        }
        else
        {
            glGenBuffers(1, vbo);
            glBindBuffer(GL_ARRAY_BUFFER, *vbo);
        }

        glBufferData(GL_ARRAY_BUFFER, (size_t)vertices * 3 * sizeof(float), positions, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        free(positions);

        return vao;
    }

    return 0;
}

std::string TimeNow_HHMMSS()
{
    using namespace std::chrono;

    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    char buf[9]; // HH:MM:SS
    std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
        tm.tm_hour, tm.tm_min, tm.tm_sec);

    return std::string(buf);
}

// IO
bool LoadFileBinary(std::string file, std::vector<char>* out)
{
    // Create the stream
    std::ifstream infile(file, std::ios::binary | std::ios::ate);

    // Check if file opened
    if (infile.good())
    {
        // Get length of the file
        size_t length = (size_t)infile.tellg();

        // Move to begining
        infile.seekg(0, std::ios::beg);

        // Resize the vector
        out->resize(length * sizeof(char));

        // Read into the buffer
        infile.read(out->data(), length);

        // Close the stream
        infile.close();

        return true;
    }
    else
    {
        return false;
    }
}

std::string LoadFile(std::string file)
{
    std::string ret;

    std::string line;
    std::ifstream inFile(file);
    if (inFile.good())
    {
        while (getline(inFile, line))
        {
            if (line.length() > 0)
            {
                if (line.find("$$INC$$:") != std::string::npos)
                {
                    std::ifstream inFile2(GetRelPath("res/shaders/") + line.substr(line.find("$$INC$$:") + std::string("$$INC$$:").length()));
                    if (inFile2.good())
                    {
                        while (getline(inFile2, line))
                        {
                            ret.append(line + "\n");
                        }
                        inFile2.close();
                    }
                }
                else
                {
                    ret.append(line + "\n");
                }
            }
        }
        inFile.close();
    }

    return ret;
}



// Crypt
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
std::string HashSHA256(const std::string& input)
{
    using namespace CryptoPP;

    SHA256 hash;
    std::string digest;

    StringSource ss(input, true,
        new HashFilter(hash,
            new HexEncoder(
                new StringSink(digest), false // uppercase=false -> lowercase hex
            )
        )
    );

    return digest;
}

#include <iostream>
#include <filesystem>
#include <stdio.h>
#undef APIENTRY
#include <windows.h>
#include <dbghelp.h>



void CreateMiniDump(EXCEPTION_POINTERS* e)
{
    SYSTEMTIME st;
    GetSystemTime(&st);
    char filename[MAX_PATH];
    sprintf_s(filename, "Crash_%04d%02d%02d_%02d%02d%02d.dmp", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    HANDLE hFile = CreateFileA(filename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, nullptr);

    if ((hFile != nullptr) && (hFile != INVALID_HANDLE_VALUE)) {
        MINIDUMP_EXCEPTION_INFORMATION info{};
        info.ThreadId = GetCurrentThreadId();
        info.ExceptionPointers = e;
        info.ClientPointers = FALSE;

        MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            MiniDumpWithFullMemory, // or MiniDumpNormal
            e ? &info : nullptr,
            nullptr,
            nullptr
        );

        CloseHandle(hFile);
        std::cout << "Crash dump written: " << filename << std::endl;
    }
}

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* e)
{
    std::cerr << "Unhandled exception caught! Creating dump...\n";
    CreateMiniDump(e);
    return EXCEPTION_EXECUTE_HANDLER;
}

void EasyUtils_Init()
{
    SetUnhandledExceptionFilter(ExceptionHandler);
}

std::string GetRelPath(std::string append)
{
    static std::string root;
    static bool i = true;
    if (i)
    {
        i = false;
        char buffer[128];
        GetModuleFileNameA(NULL, buffer, 128);
        std::string fullPath(buffer);
        for (auto& c : fullPath)
            if (c == '\\')
                c = '/';
        size_t pos = fullPath.find_last_of("\\/");
        root = fullPath.substr(0, pos) + '/';

        bool resFolder = std::filesystem::exists(root + "res") && std::filesystem::is_directory(root + "res");
        if (!resFolder)
        {
            root = root + "../";
            resFolder = std::filesystem::exists(root + "res") && std::filesystem::is_directory(root + "res");
        }

        if (!resFolder)
        {
            root = root + "../";
            resFolder = std::filesystem::exists(root + "res") && std::filesystem::is_directory(root + "res");
        }

        if (!resFolder)
        {
            root = root + "../";
            resFolder = std::filesystem::exists(root + "res") && std::filesystem::is_directory(root + "res");
        }
    }

    return root + append;
}
