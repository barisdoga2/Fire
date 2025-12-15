#include "pch.h"
#include "EasyIO.hpp"
#include "EasyDisplay.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>



void EasyMouse::Init(const EasyDisplay& display)
{
	listeners.clear();

	glfwSetMouseButtonCallback(display.window, mouse_callback);
	glfwSetCursorPosCallback(display.window, cursor_callback);
	glfwSetScrollCallback(display.window, scroll_callback);

	glfwGetCursorPos(display.window, &data.position.now.x, &data.position.now.y);

	data.window = display.window;
	data.position.old = data.position.now;
}

void EasyMouse::AddListener(MouseListener* listener)
{
	listeners.push_back(listener);
}

bool EasyMouse::ListenButton(int button)
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

bool EasyMouse::IsButtonDown(int button)
{
	if (button < MAX_BUTTONS)
		return buttons[button] != 0;
	else
		return false;
}

void EasyMouse::mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button < MAX_BUTTONS)
		buttons[button] = action;

	data.button.button = button;
	data.button.action = action;
	data.button.mods = mods;

	for (MouseListener* l : listeners)
	{
		if (l->mouse_callback(data))
			return;
	}
}

void EasyMouse::cursor_callback(GLFWwindow* window, double xpos, double ypos)
{
	data.position.old = data.position.now;
	data.position.now.x = xpos;
	data.position.now.y = ypos;
	data.position.delta = data.position.now - data.position.old;
	// screenPosition = ToScreenCoords(data.position.now);

	for (MouseListener* l : listeners)
	{
		if (l->cursorMove_callback(data))
			return;
	}
}

void EasyMouse::scroll_callback(GLFWwindow* window, double posX, double posY)
{
	data.scroll.now.x = posX;
	data.scroll.now.y = posY;

	for (MouseListener* l : listeners)
	{
		if (l->scroll_callback(data))
			return;
	}
}

void EasyKeyboard::Init(const EasyDisplay& display)
{
	listeners.clear();

	glfwSetKeyCallback(display.window, key_callback);
	glfwSetCharCallback(display.window, character_callback);

	data.window = display.window;
}

void EasyKeyboard::AddListener(KeyboardListener* listener)
{
	listeners.push_back(listener);
}

bool EasyKeyboard::ListenKey(int key)
{
	bool isKey = IsKeyDown(key);
	if (isKey && keys_read[key])
	{
		keys_read[key] = false;
		return true;
	}
	else if (!isKey)
	{
		keys_read[key] = true;
		return false;
	}
	return false;
}

bool EasyKeyboard::IsKeyDown(int key)
{
	if (key < MAX_KEYS)
		return keys[key] != 0;
	else
		return false;
}

void EasyKeyboard::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key < MAX_KEYS)
		keys[key] = action;

	data.key = key;
	data.scancode = scancode;
	data.action = action;
	data.mods = mods;

	for (KeyboardListener* k : listeners)
		if (k->key_callback(data))
			return;
}

void EasyKeyboard::character_callback(GLFWwindow* window, unsigned int codepoint)
{
	data.codepoint = codepoint;

	for (KeyboardListener* k : listeners)
		if (k->character_callback(data))
			return;
}

int EasyKeyboard::keys[MAX_KEYS] = { 0 };
bool EasyKeyboard::keys_read[MAX_KEYS] = { 0 };
KeyboardData EasyKeyboard::data;
std::vector<KeyboardListener*> EasyKeyboard::listeners;

int EasyMouse::buttons[MAX_BUTTONS] = { 0 };
bool EasyMouse::buttons_read[MAX_BUTTONS];
std::vector<MouseListener*> EasyMouse::listeners;
MouseData EasyMouse::data;