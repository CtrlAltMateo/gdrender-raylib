#include "RenderTexture.h"

std::shared_ptr<GameRenderTexture> GameRenderTexture::create(glm::vec2 resolution)
{
	std::shared_ptr<GameRenderTexture> ptr = std::make_shared<GameRenderTexture>();

	if (ptr->init(resolution))
		return ptr;

	return nullptr;
}

bool GameRenderTexture::init(glm::vec2 resolution)
{
	resize(resolution);
	return IsRenderTextureValid(target);
}

void GameRenderTexture::resize(glm::vec2 resolution)
{
	if (IsRenderTextureValid(target))
		UnloadRenderTexture(target);
	target = LoadRenderTexture(static_cast<int>(resolution.x), static_cast<int>(resolution.y));
	this->resolution = resolution;
}

void GameRenderTexture::bind()
{
	BeginTextureMode(target);
}

void GameRenderTexture::unbind()
{
	EndTextureMode();
}

void GameRenderTexture::clear()
{
	ClearBackground(BLACK);
}