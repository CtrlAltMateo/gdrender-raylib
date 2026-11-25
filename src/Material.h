#pragma once

#include "Texture.h"

#include <memory>
#include <string>
#include <vector>

class GameMaterial
{
  private:
	std::vector<std::shared_ptr<GameTexture>> textures;

  public:
	static int id;

  public:
	static std::shared_ptr<GameMaterial> create();
	bool init();

	void addTexture(std::shared_ptr<GameTexture> tex, uint32_t slot);

	std::vector<std::shared_ptr<GameTexture>>& getTextures()
	{
		return textures;
	}
};