#include "Texture.h"

std::unordered_map<std::string, std::shared_ptr<GameTexture>> GameTexture::cachedTextures;

std::shared_ptr<GameTexture> GameTexture::create(const std::string& path)
{
	if (cachedTextures.contains(path))
		return cachedTextures[path];

	std::shared_ptr<GameTexture> ptr = std::make_shared<GameTexture>();

	if (ptr->init(path))
	{
		cachedTextures.insert({path, ptr});
		return ptr;
	}

	return nullptr;
}

bool GameTexture::init(const std::string& path)
{
	textureID = LoadTexture(path.c_str());
	if (!IsTextureValid(textureID))
		return false;

	resolution = {static_cast<float>(textureID.width), static_cast<float>(textureID.height)};
	SetTextureWrap(textureID, TEXTURE_WRAP_REPEAT);
	SetTextureFilter(textureID, TEXTURE_FILTER_TRILINEAR);

	return true;
}