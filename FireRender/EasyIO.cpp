#include "pch.h"
#include "EasyIO.hpp"
#include "EasyDisplay.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>



void EasyMouse::Init()
{
	DeInit();

	glfwSetMouseButtonCallback(EasyDisplay::GetWindow(), button_callback);
	glfwSetCursorPosCallback(EasyDisplay::GetWindow(), move_callback);
	glfwSetScrollCallback(EasyDisplay::GetWindow(), scroll_callback);
	glfwSetCursorEnterCallback(EasyDisplay::GetWindow(), enter_callback);

	glfwGetCursorPos(EasyDisplay::GetWindow(), &data.position.now.x, &data.position.now.y);

	data.window = EasyDisplay::GetWindow();
	data.position.old = data.position.now;
}

void EasyMouse::DeInit()
{
	glfwSetMouseButtonCallback(EasyDisplay::GetWindow(), nullptr);
	glfwSetCursorPosCallback(EasyDisplay::GetWindow(), nullptr);
	glfwSetScrollCallback(EasyDisplay::GetWindow(), nullptr);
	glfwSetCursorEnterCallback(EasyDisplay::GetWindow(), nullptr);

	listeners = {};
	data = {};
	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		buttons[i] = false;
		buttons_read[i] = false;
	}
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

void EasyMouse::button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button < MAX_BUTTONS)
		buttons[button] = action;

	data.button.button = button;
	data.button.action = action;
	data.button.mods = mods;

	for (MouseListener* l : listeners)
	{
		if (l->button_callback(data))
			return;
	}
}

void EasyMouse::move_callback(GLFWwindow* window, double xpos, double ypos)
{
	data.position.old = data.position.now;
	data.position.now.x = xpos;
	data.position.now.y = ypos;
	data.position.delta = data.position.now - data.position.old;
	// screenPosition = ToScreenCoords(data.position.now);

	for (MouseListener* l : listeners)
	{
		if (l->move_callback(data))
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

void EasyMouse::enter_callback(GLFWwindow* window, int entered)
{
	data.position.entered = entered;

	for (MouseListener* l : listeners)
	{
		if (l->enter_callback(data))
			return;
	}
}

void EasyKeyboard::Init()
{
	DeInit();

	glfwSetKeyCallback(EasyDisplay::GetWindow(), key_callback);
	glfwSetCharCallback(EasyDisplay::GetWindow(), char_callback);
	glfwSetCharModsCallback(EasyDisplay::GetWindow(), char_mods_callback);

	data.window = EasyDisplay::GetWindow();
}

void EasyKeyboard::DeInit()
{
	glfwSetKeyCallback(EasyDisplay::GetWindow(), nullptr);
	glfwSetCharCallback(EasyDisplay::GetWindow(), nullptr);
	glfwSetCharModsCallback(EasyDisplay::GetWindow(), nullptr);

	listeners = {};
	data = {};
	for (int i = 0; i < MAX_BUTTONS; i++)
	{
		keys[i] = false;
		keys_read[i] = false;
	}
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

void EasyKeyboard::char_callback(GLFWwindow* window, unsigned int codepoint)
{
	data.codepoint = codepoint;

	for (KeyboardListener* k : listeners)
		if (k->char_callback(data))
			return;
}

void EasyKeyboard::char_mods_callback(GLFWwindow* window, unsigned int codepoint, int mods)
{
	data.codepoint = codepoint;
	data.mods = mods;

	for (KeyboardListener* k : listeners)
		if (k->char_mods_callback(data))
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
