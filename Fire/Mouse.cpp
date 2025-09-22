#include "Mouse.hpp"
#include <GLFW/glfw3.h>

void Mouse::Init(GLFWwindow* window)
{
	glfwSetMouseButtonCallback(window, mouse_callback);
	glfwSetCursorPosCallback(window, cursor_callback);
	glfwSetScrollCallback(window, scroll_callback);

	glfwGetCursorPos(window, &data.position.now.x, &data.position.now.y);

	data.window = window;
	data.position.old = data.position.now;
}

void Mouse::AddListener(MouseListener* listener)
{
	listeners.push_back(listener);
}

bool Mouse::ListenButton(int button)
{
	bool isButton = IsButtonDown(button);
	if (isButton && buttons_read[button])
	{
		buttons_read[button] = false;
		return true;
	}
	else if (!isButton)
	{
		buttons_read[button] = true;
		return false;
	}
	return false;
}

bool Mouse::IsButtonDown(int button)
{
	if (button < MAX_BUTTONS)
		return buttons[button] != 0;
	else
		return false;
}

void Mouse::mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button < MAX_BUTTONS)
		buttons[button] = action;

	data.button.button = button;
	data.button.action = action;
	data.button.mods = mods;

	for (MouseListener* l : listeners)
	{
		if (l->mouse_callback() || l->mouse_callback(data) || l->mouse_callback(data.window, data.button.button, data.button.action, data.button.mods))
			return;
	}
}

void Mouse::cursor_callback(GLFWwindow* window, double xpos, double ypos)
{
	data.position.old = data.position.now;
	data.position.now.x = xpos;
	data.position.now.y = ypos;
	data.position.delta = data.position.now - data.position.old;
	// screenPosition = ToScreenCoords(data.position.now);

	for (MouseListener* l : listeners)
	{
		if (l->cursorMove_callback() || l->cursorMove_callback(data) || l->cursorMove_callback(data.position.delta))
			return;
	}
}

void Mouse::scroll_callback(GLFWwindow* window, double posX, double posY)
{

	data.scroll.now.x = posX;
	data.scroll.now.y = posY;

	for (MouseListener* l : listeners)
	{
		if (l->scroll_callback() || l->scroll_callback(data) || (l->scroll_callback(data.window, data.scroll.now.x, data.scroll.now.y)))
			return;
	}
}

int Mouse::buttons[MAX_BUTTONS] = { 0 };
bool Mouse::buttons_read[MAX_BUTTONS];
std::vector<MouseListener*> Mouse::listeners;
MouseData Mouse::data;
