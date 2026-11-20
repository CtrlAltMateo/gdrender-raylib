#include "Scrolling.h"

#include "Texture.h"

#include <algorithm>
#include <format>

namespace
{

constexpr float blockSize = 30.0f;
constexpr float parallax = 0.1f;

float GetBackgroundScale(Texture2D texture)
{
	const float screenWidth = static_cast<float>(GetScreenWidth());
	const float screenHeight = static_cast<float>(GetScreenHeight());
	return std::max(screenWidth / static_cast<float>(texture.width),
					screenHeight / static_cast<float>(texture.height));
}

}

Scrolling::~Scrolling()
{
	unload();
}

bool Scrolling::load(int backgroundID)
{
	unload();

	background = GameTexture::create(std::format("Resources/game_bg_{:02d}_001-uhd.png", backgroundID));
	if (!background)
	{
		TraceLog(LOG_ERROR, "Could not load background %d", backgroundID);
		return false;
	}

	ground = LoadTexture("Assets/Textures/ground.png");
	if (!IsTextureValid(ground))
	{
		TraceLog(LOG_ERROR, "Could not load Assets/Textures/ground.png");
		background.reset();
		return false;
	}

	SetTextureFilter(ground, TEXTURE_FILTER_BILINEAR);
	backgroundOffset = 0.0f;
	groundOffset = 0.0f;
	return true;
}

void Scrolling::update(float cameraMovementX, float cameraZoom)
{
	if (cameraZoom <= 0.0f)
		return;

	if (background)
	{
		const Texture2D texture = background->getTexture();
		const float backgroundWidth = texture.width * GetBackgroundScale(texture);
		backgroundOffset -= cameraMovementX * cameraZoom * parallax;
		if (backgroundWidth > 0.0f && backgroundOffset <= -backgroundWidth)
			backgroundOffset = 0.0f;
	}

	const float groundWidth = blockSize * cameraZoom;
	groundOffset -= cameraMovementX * cameraZoom;
	if (groundWidth > 0.0f && groundOffset <= -groundWidth)
		groundOffset = 0.0f;
}

void Scrolling::draw(const Camera2D& camera) const
{
	const float zoom = camera.zoom > 0.0f ? camera.zoom : 1.0f;
	if (background)
	{
		const Texture2D texture = background->getTexture();
		const float scale = GetBackgroundScale(texture);
		const float backgroundWidth = texture.width * scale;
		const float backgroundY =
			(static_cast<float>(GetScreenHeight()) - texture.height * scale) * 0.5f;
		if (backgroundWidth > 0.0f)
		{
			for (float x = backgroundOffset; x < GetScreenWidth(); x += backgroundWidth)
				DrawTextureEx(texture, {x, backgroundY}, 0.0f, scale, backgroundColor);
		}
	}

	if (!IsTextureValid(ground))
		return;

	const float groundY = (0.0f - camera.target.y) * zoom + camera.offset.y;
	const float tileSize = blockSize * zoom;
	if (groundY >= GetScreenHeight() || tileSize <= 0.0f)
		return;

	const float scale = tileSize / static_cast<float>(ground.width);
	for (float y = groundY; y < GetScreenHeight(); y += tileSize)
		for (float x = groundOffset; x < GetScreenWidth(); x += tileSize)
			DrawTextureEx(ground, {x, y}, 0.0f, scale, groundColor);
}

void Scrolling::setBackgroundColor(Color color)
{
	backgroundColor = color;
}

void Scrolling::setGroundColor(Color color)
{
	groundColor = color;
}

void Scrolling::unload()
{
	if (IsTextureValid(ground))
		UnloadTexture(ground);
	ground = {};
	background.reset();
	backgroundOffset = 0.0f;
	groundOffset = 0.0f;
}
