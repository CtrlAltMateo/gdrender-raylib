#include "Input.h"

#include <raylib.h>

glm::vec2 Input::lastMousePosition = {0, 0};
glm::vec2 Input::mousePosition = {0, 0};
glm::vec2 Input::mouseDelta = {0, 0};
float Input::scrollDelta;

std::unordered_map<int, bool> Input::mouseButtons;
std::unordered_map<int, bool> Input::keyMap;

void Input::init()
{
	Vector2 position = GetMousePosition();
	lastMousePosition = {position.x, position.y};
	mousePosition = lastMousePosition;
}

void Input::update()
{
	Vector2 position = GetMousePosition();
	Vector2 delta = GetMouseDelta();
	mousePosition = {position.x, position.y};
	mouseDelta = {delta.x, -delta.y};
	scrollDelta = GetMouseWheelMove();
	Input::lastMousePosition = Input::mousePosition;

	keyMap[KEY_ESCAPE] = IsKeyDown(KEY_ESCAPE);
	keyMap[KEY_F] = IsKeyDown(KEY_F);
	keyMap[KEY_LEFT_SHIFT] = IsKeyDown(KEY_LEFT_SHIFT);
	keyMap[KEY_W] = IsKeyDown(KEY_W);
	keyMap[KEY_S] = IsKeyDown(KEY_S);

	mouseButtons[MOUSE_BUTTON_LEFT] = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}

void Input::postUpdate()
{
	scrollDelta = 0;
}