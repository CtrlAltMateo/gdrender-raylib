#include "Material.h"

int GameMaterial::id;

std::shared_ptr<GameMaterial> GameMaterial::create()
{
	std::shared_ptr<GameMaterial> ptr = std::make_shared<GameMaterial>();
	if (ptr->init())
		return ptr;

	return nullptr;
}

bool GameMaterial::init()
{
	id++;
	return true;
}

void GameMaterial::addTexture(std::shared_ptr<GameTexture> tex, uint32_t slot)
{
	this->textures.push_back(tex);
	tex->textureSlot = slot;
}