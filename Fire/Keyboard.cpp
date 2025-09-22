#include "Keyboard.hpp"
#include <GLFW/glfw3.h>

void Keyboard::Init(GLFWwindow* window)
{
	glfwSetKeyCallback(window, key_callback);
	glfwSetCharCallback(window, character_callback);

	data.window = window;
}

void Keyboard::AddListener(KeyboardListener* listener)
{
	listeners.push_back(listener);
}

bool Keyboard::ListenKey(int key)
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

bool Keyboard::IsKeyDown(int key)
{
	if (key < MAX_KEYS)
		return keys[key] != 0;
	else
		return false;
}

void Keyboard::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key < MAX_KEYS)
		keys[key] = action;

	data.key = key;
	data.scancode = scancode;
	data.action = action;
	data.mods = mods;

	for (KeyboardListener* k : listeners)
		if (k->key_callback() || k->key_callback(data) || k->key_callback(window, key, scancode, action, mods))
			return;
}

void Keyboard::character_callback(GLFWwindow* window, unsigned int codepoint)
{
	data.codepoint = codepoint;

	for (KeyboardListener* k : listeners)
		if (k->character_callback() || k->character_callback(data) || k->character_callback(window, codepoint))
			return;
}

int Keyboard::keys[MAX_KEYS] = { 0 };
bool Keyboard::keys_read[MAX_KEYS] = { 0 };
KeyboardData Keyboard::data;
std::vector<KeyboardListener*> Keyboard::listeners;