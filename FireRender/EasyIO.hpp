#pragma once

#include <glm/glm.hpp>
#include <vector>

#define MAX_BUTTONS 3U
#define MAX_KEYS 512U


class EasyListener {
public:
	bool isEnabled{true};
	int z{0};
};


class Button {
public:
	int button = 0;
	int action = 0;
	int mods = 0;
};

class Position {
public:
	glm::dvec2 old = { 0, 0 };
	glm::dvec2 now = { 0, 0 };
	glm::dvec2 delta = { 0, 0 };
	int entered = 0;
};

class Scroll {
public:
	glm::dvec2 now = { 0, 0 };
};

struct GLFWwindow;
class MouseData {
public:
	GLFWwindow* window{};
	Button button{};
	Position position{};
	Scroll scroll{};
	bool cursorEnabled{true};
};

class MouseListener : public EasyListener {
public:
	virtual inline bool button_callback(const MouseData& data) { return false; }
	virtual inline bool scroll_callback(const MouseData& data) { return false; }
	virtual inline bool move_callback(const MouseData& data) { return false; }
	virtual inline bool enter_callback(const MouseData& data) { return false; }
};

class EasyMouse {
private:
	EasyMouse() = delete;
	EasyMouse(EasyMouse&) = delete;

	static int buttons[MAX_BUTTONS];
	static bool buttons_read[MAX_BUTTONS];
	static MouseData data;
	static std::vector<MouseListener*> listeners;
public:
	static void Init();
	static void DeInit();

	static void Update(double _dt);

	static void EnableCursor(bool enabled);
	static void AddListener(MouseListener* listener);
	static bool ListenButton(int button);
	static bool IsButtonDown(int button);

private:
	static void button_callback(GLFWwindow* window, int button, int action, int mods);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void move_callback(GLFWwindow* window, double xpos, double ypos);
	static void enter_callback(GLFWwindow* window, int entered);
};




class KeyboardData {
public:
	GLFWwindow* window;
	int key = 0;
	int scancode = 0;
	int action = 0;
	int mods = 0;
	unsigned int codepoint = 0;
};

class KeyboardListener : public EasyListener {
public:
	virtual inline bool key_callback(const KeyboardData& data) { return false; }
	virtual inline bool char_callback(const KeyboardData& data) { return false; }
	virtual inline bool char_mods_callback(const KeyboardData& data) { return false; }
};

class EasyKeyboard {
private:
	EasyKeyboard() = delete;
	EasyKeyboard(EasyKeyboard&) = delete;

	static int keys[MAX_KEYS];
	static bool keys_read[MAX_KEYS];
	static KeyboardData data;
	static std::vector<KeyboardListener*> listeners;
public:
	static void Init();
	static void DeInit();

	static void Update(double _dt);

	static void AddListener(KeyboardListener* listener);
	static bool ListenKey(int key);
	static bool IsKeyDown(int key);

private:
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void char_callback(GLFWwindow* window, unsigned int codepoint);
	static void char_mods_callback(GLFWwindow* window, unsigned int codepoint, int mods);
};

namespace EasyIO {
	static inline void Init()
	{
		EasyKeyboard::Init();
		EasyMouse::Init();
	}

	static inline void DeInit()
	{
		EasyKeyboard::DeInit();
		EasyMouse::DeInit();
	}
}
