#include "Scrolling.h"

#include "Texture.h"

#include <algorithm>
#include <format>

namespace
{

constexpr float blockSize = 30.0f;
constexpr float groundTileBlocks = 4.0f;
constexpr float parallax = 0.1f;
constexpr float backgroundScaleOverscan = 1.1f;

float GetBackgroundScale(Texture2D texture)
{
	const float screenWidth = static_cast<float>(GetScreenWidth());
	const float screenHeight = static_cast<float>(GetScreenHeight());
	return std::max(screenWidth / static_cast<float>(texture.width),
					screenHeight / static_cast<float>(texture.height)) * backgroundScaleOverscan;
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

	ground = LoadTexture("Assets/Textures/Game/ground.png");
	if (!IsTextureValid(ground))
	{
		TraceLog(LOG_ERROR, "Could not load Assets/Textures/Game/ground.png");
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

	const float groundWidth = blockSize * groundTileBlocks * cameraZoom;
	groundOffset -= cameraMovementX * cameraZoom;
	if (groundWidth > 0.0f && groundOffset <= -groundWidth)
		groundOffset = 0.0f;
}

void Scrolling::draw(const Camera2D& camera, float groundScreenY, float cameraVerticalOffset) const
{
	const float zoom = camera.zoom > 0.0f ? camera.zoom : 1.0f;
	if (background)
	{
		const Texture2D texture = background->getTexture();
		const float scale = GetBackgroundScale(texture);
		const float backgroundWidth = texture.width * scale;
		const float backgroundHeight = texture.height * scale;
		const float height = static_cast<float>(GetScreenHeight());
		const float centeredY = (height - backgroundHeight) * 0.5f;
		const float backgroundY = std::clamp(
			centeredY + cameraVerticalOffset * parallax,
			height - backgroundHeight,
			0.0f);
		if (backgroundWidth > 0.0f)
		{
			for (float x = backgroundOffset; x < GetScreenWidth(); x += backgroundWidth)
				DrawTextureEx(texture, {x, backgroundY}, 0.0f, scale, backgroundColor);
		}
	}

	if (!IsTextureValid(ground))
		return;

	const float tileWidth = blockSize * groundTileBlocks * zoom;
	const float tileHeight = blockSize * zoom;
	if (groundScreenY >= GetScreenHeight() || tileWidth <= 0.0f || tileHeight <= 0.0f)
		return;

	const Rectangle source = {
		0.0f, 0.0f, static_cast<float>(ground.width), static_cast<float>(ground.height)
	};
	for (float x = groundOffset; x < GetScreenWidth(); x += tileWidth)
	{
		DrawTexturePro(
			ground,
			source,
			{x, groundScreenY, tileWidth, tileHeight},
			{0.0f, 0.0f},
			0.0f,
			groundColor);
	}
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
