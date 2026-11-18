#pragma once

#include <iostream>
#include <glm/vec2.hpp>
#include <raylib.h>
#include <memory>

class GameRenderTexture
{
  private:
	RenderTexture2D target = {0};

  public:
    glm::vec2 resolution = {0, 0};

  public:

    static std::shared_ptr<GameRenderTexture> create(glm::vec2 resolution);
    bool init(glm::vec2 resolution);

    void resize(glm::vec2 resolution);

    void bind();
    void unbind();

    void clear();

	Texture2D getTexture() const
	{
		return target.texture;
	}
};