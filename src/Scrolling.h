#pragma once

#include <raylib.h>

#include <memory>

class GameTexture;

class Scrolling
{
  private:
	std::shared_ptr<GameTexture> background;
	Texture2D ground{};
	float backgroundOffset = 0.0f;
	float groundOffset = 0.0f;
	Color backgroundColor = WHITE;
	Color groundColor = WHITE;

  public:
	~Scrolling();

	bool load(int backgroundID);
	void update(float cameraMovementX, float cameraZoom);
	void draw(const Camera2D& camera) const;
	void setBackgroundColor(Color color);
	void setGroundColor(Color color);
	void unload();
};
