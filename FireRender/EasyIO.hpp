#pragma once

#include <glm/glm.hpp>
#include <vector>

#define MAX_BUTTONS 3U
#define MAX_KEYS 1024U

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
};

class Scroll {
public:
	glm::dvec2 now = { 0, 0 };
};

struct GLFWwindow;
class MouseData {
public:
	GLFWwindow* window;
	Button button;
	Position position;
	Scroll scroll;
};

class MouseListener {
public:
	virtual inline bool mouse_callback(const MouseData& md) { return false; }
	virtual inline bool scroll_callback(const MouseData& md) { return false; }
	virtual inline bool cursorMove_callback(const MouseData& md) { return false; }
};

class EasyDisplay;
class EasyMouse : MouseListener {
private:
	static int buttons[MAX_BUTTONS];
	static bool buttons_read[MAX_BUTTONS];
public:
	static MouseData data;
	static std::vector<MouseListener*> listeners;
	static void Init(const EasyDisplay& display);
	static void AddListener(MouseListener* listener);
	static bool ListenButton(int button);
	static bool IsButtonDown(int button);
	static void mouse_callback(GLFWwindow* window, int button, int action, int mods);
	static void cursor_callback(GLFWwindow* window, double xpos, double ypos);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
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

class KeyboardListener {
public:
	virtual bool key_callback(const KeyboardData& data) { return false; }
	virtual bool character_callback(const KeyboardData& data) { return false; }
};

class EasyKeyboard {
private:
	static int keys[MAX_KEYS];
	static bool keys_read[MAX_KEYS];

public:
	static KeyboardData data;
	static std::vector<KeyboardListener*> listeners;
	static void Init(const EasyDisplay& display);
	static void AddListener(KeyboardListener* listener);
	static bool ListenKey(int key);
	static bool IsKeyDown(int key);
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void character_callback(GLFWwindow* window, unsigned int codepoint);
};
