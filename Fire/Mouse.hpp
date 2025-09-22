#pragma once

#include <glm/glm.hpp>
#include <vector>

#define MAX_BUTTONS 16

struct GLFWwindow;

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

class MouseData {
public:
	GLFWwindow* window;
	Button button;
	Position position;
	Scroll scroll;

};

class MouseListener {
public:
	virtual inline bool mouse_callback() { return false; }
	virtual inline bool mouse_callback(const MouseData& md) { return false; }
	virtual inline bool mouse_callback(GLFWwindow* window, int button, int action, int mods) { return false; };

	virtual inline bool scroll_callback() { return false; }
	virtual inline bool scroll_callback(const MouseData& md) { return false; }
	virtual inline bool scroll_callback(GLFWwindow* window, double x, double y) { return false; };

	virtual inline bool cursorMove_callback() { return false; }
	virtual inline bool cursorMove_callback(const MouseData& md) { return false; }
	virtual inline bool cursorMove_callback(glm::vec2 normalizedDelta, bool checkCursor = true) { return false; };

};

class Mouse : MouseListener {
private:
	static int buttons[MAX_BUTTONS];
	static bool buttons_read[MAX_BUTTONS];

public:
	static MouseData data;

	static std::vector<MouseListener*> listeners;

	static void Init(GLFWwindow* window);

	static void AddListener(MouseListener* listener);

	static bool ListenButton(int button);

	static bool IsButtonDown(int button);

	static void mouse_callback(GLFWwindow* window, int button, int action, int mods);

	static void cursor_callback(GLFWwindow* window, double xpos, double ypos);

	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);


};
