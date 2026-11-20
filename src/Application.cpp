#include "Application.h"

#include <raylib.h>

Application* Application::instance;
extern Camera2D camera;

namespace
{

RenderTexture2D mirrorTarget{};

bool EnsureMirrorTarget()
{
	const int width = GetScreenWidth();
	const int height = GetScreenHeight();
	if (IsRenderTextureValid(mirrorTarget) && mirrorTarget.texture.width == width &&
		mirrorTarget.texture.height == height)
		return true;

	if (IsRenderTextureValid(mirrorTarget))
		UnloadRenderTexture(mirrorTarget);
	mirrorTarget = LoadRenderTexture(width, height);
	if (!IsRenderTextureValid(mirrorTarget))
	{
		TraceLog(LOG_ERROR, "Could not create mirror render target");
		return false;
	}
	return true;
}

void DrawScene(Layer* layer, void (*overlay)())
{
	if (layer)
		layer->drawBackground();
	BeginMode2D(camera);
	if (layer)
		layer->draw();
	if (overlay)
		overlay();
	EndMode2D();
}

}

void Application::update(float frameDelta)
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

	deltaTime = frameDelta >= 0.0f ? frameDelta : GetFrameTime();
	totalTime += deltaTime;

	if (currentLayer)
		currentLayer->update();
}

void Application::draw(void (*overlay)())
{
	BeginDrawing();
	ClearBackground(BLACK);

	if (mirrorX && EnsureMirrorTarget())
	{
		BeginTextureMode(mirrorTarget);
		ClearBackground(BLACK);
		DrawScene(currentLayer.get(), overlay);
		EndTextureMode();
		DrawTexturePro(mirrorTarget.texture,
					   {static_cast<float>(mirrorTarget.texture.width), 0.0f,
						-static_cast<float>(mirrorTarget.texture.width),
						-						-static_cast<float>(mirrorTarget.texture.height)},
					   {0.0f, 0.0f, static_cast<float>(GetScreenWidth()),
						static_cast<float>(GetScreenHeight())},
					   {0.0f, 0.0f}, 0.0f, WHITE);
	}
	else
		DrawScene(currentLayer.get(), overlay);

	EndDrawing();
}

void Application::exit()
{
	if (IsRenderTextureValid(mirrorTarget))
	{
		UnloadRenderTexture(mirrorTarget);
		mirrorTarget = {};
	}
	if (currentLayer)
		popLayer();
	pendingLayer.reset();
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