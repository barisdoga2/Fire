#pragma once

#include <vector>

#define MAX_KEYS 1024

struct GLFWwindow;

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
	virtual bool key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) { return false; }
	virtual bool key_callback(const KeyboardData& data) { return false; }
	virtual bool key_callback() { return false; }

	virtual bool character_callback(GLFWwindow* window, unsigned int codepoint) { return false; }
	virtual bool character_callback(const KeyboardData& data) { return false; }
	virtual bool character_callback() { return false; }
};

class Keyboard {
private:
	static int keys[MAX_KEYS];
	static bool keys_read[MAX_KEYS];

public:
	static KeyboardData data;

	static std::vector<KeyboardListener*> listeners;

	static void Init(GLFWwindow* window);

	static void AddListener(KeyboardListener* listener);

	static bool ListenKey(int key);

	static bool IsKeyDown(int key);

	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

	static void character_callback(GLFWwindow* window, unsigned int codepoint);
};
