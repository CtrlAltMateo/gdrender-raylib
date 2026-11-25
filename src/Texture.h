#pragma once

#include <raylib.h>

#include <iostream>
#include <memory>
#include <unordered_map>
#include <glm/vec2.hpp>

class GameTexture
{
  private:
	Texture2D textureID = {0};

	static std::unordered_map<std::string, std::shared_ptr<GameTexture>> cachedTextures;

  public:
	  uint32_t textureSlot = 0;

	  glm::vec2 resolution = {0, 0};

  public:
	static std::shared_ptr<GameTexture> create(const std::string& path);

	bool init(const std::string& path);

	Texture2D getTexture() const
	{
		return this->textureID;
	}
};