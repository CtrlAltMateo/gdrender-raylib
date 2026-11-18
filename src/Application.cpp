#include "Application.h"
#include "Input.h"

#include <raylib.h>

Application* Application::instance;

void Application::init(glm::vec2 resolution, std::string name)
{
	instance = this;
	this->resolution = resolution;

	SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_FULLSCREEN_MODE);
	InitWindow(resolution.x, resolution.y, name.c_str());
	SetExitKey(KEY_NULL);

	Input::init();

	lastFrame = GetTime();
}

void Application::startUpdate()
{
	while (!WindowShouldClose())
	{
		update();

		draw();
	}

	exit();
}

void Application::update()
{
	if (IsWindowResized())
		resolution = {GetScreenWidth(), GetScreenHeight()};

	if (pendingLayer)
	{
		if (currentLayer)
			popLayer();

		currentLayer = pendingLayer;
		pendingLayer = nullptr;
	}

	float currentFrame = GetTime();

	deltaTime = currentFrame - lastFrame;
	lastFrame = currentFrame;

	totalTime += deltaTime;

	Input::update();

	if (currentLayer)
		currentLayer->update();

	Input::postUpdate();
}

void Application::draw()
{
	BeginDrawing();
	ClearBackground(BLACK);

	if (currentLayer)
		currentLayer->draw();

	if (currentLayer)
		currentLayer->GUI();

	EndDrawing();
}

void Application::exit()
{
	if (currentLayer)
		popLayer();

	CloseWindow();
}

void Application::pushLayer(std::shared_ptr<Layer> layer)
{
	pendingLayer = layer;
}

void Application::popLayer()
{
	currentLayer->exit();
	currentLayer = nullptr;
}