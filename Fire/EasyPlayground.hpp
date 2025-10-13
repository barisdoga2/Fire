#pragma once

#include "EasyCamera.hpp"
#include "EasyShader.hpp"
#include "EasyModel.hpp"
#include "EasyAnimator.hpp"
#include "EasyDisplay.hpp"

static const char* VS = "\n"
"\n"
"#version 400 core\n"
"\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec2 uv;\n"
"layout (location = 2) in vec3 normal;\n"
"layout (location = 3) in vec3 tangent;\n"
"layout (location = 4) in vec3 bitangent;\n"
"layout (location = 5) in ivec4 boneIds;\n"
"layout (location = 6) in vec4 weights;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform mat4 proj;\n"
"uniform mat4 boneMatrices[200];\n"
"uniform int animated;\n"
"out vec2 pass_uv;\n"
"\n"
"void main()\n"
"{\n"
"   pass_uv = uv + vec2(normal.x, tangent.y + bitangent.z) * 0.000025;\n"
"	vec4 totalPosition = vec4(0.0f);\n"
"	if(animated == 1)\n"
"	{\n"
"		for (int i = 0; i < 4; i++)\n"
"		{\n"
"			if (boneIds[i] == -1)\n"
"				continue; \n"
"			if (boneIds[i] >= 200)\n"
"			{\n"
"				totalPosition = vec4(position, 1.0f);\n"
"				break; \n"
"			}\n"
"			vec4 localPosition = boneMatrices[boneIds[i]] * vec4(position, 1.0f); \n"
"			totalPosition += localPosition * weights[i]; \n"
"		}\n"
"	}\n"
"	else\n"
"	{\n"
"		totalPosition = vec4(position, 1.0);\n"
"	}\n"
"	gl_Position = proj * view * model * totalPosition;\n"
"}\n"
"\n";

static const char* FS = "\n"
"\n"
"#version 400 core\n"
"\n"
"in vec2 pass_uv;\n"
"uniform sampler2D diffuse;\n"
"layout (location = 0) out vec4 out_color;\n"
"\n"
"void main()\n"
"{\n"
"	out_color = vec4(texture(diffuse, pass_uv).rgb, 1.0);\n"
"}\n"
"\n";

class EasyPlayground {
private:
public:
    int exitRequested = 0;
    int animation = 1;
    bool mb1_pressed = false;
    const EasyDisplay& display_;
    EasyModel model = EasyModel("../../res/Kachujin G Rosales Skin.fbx", { "../../res/Standing Idle on Kachujin G Rosales wo Skin.fbx", "../../res/Running on Kachujin G Rosales wo Skin.fbx", "../../res/Standing Aim Idle 01 on Kachujin H Rosales wo Skin.fbx" });
    EasyShader shader = EasyShader(VS, FS);
    EasyCamera camera = EasyCamera(display_, { 3,194,166 }, { 3 - 0.15,194 - 0.44,166 - 0.895 }, 74.f, 0.01f, 1000.f);
    double fps = 0.0, ups = 0.0;

    EasyPlayground(const EasyDisplay& display);

    bool Init();

    bool Update(double _dt);
    bool Render(double _dt);

    bool OneLoop();

    void scroll_callback(GLFWwindow* window, double xpos, double ypos);
    void cursor_callback(GLFWwindow* window, double xpos, double ypos);
    void mouse_callback(GLFWwindow* window, int button, int action, int mods);
    void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void char_callback(GLFWwindow* window, unsigned int codepoint);

};
