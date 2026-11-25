#pragma once

#include <algorithm>
#include <vector>

#include "Material.h"
#include "Transformable.h"
#include "Vertex.h"
#include "Sprite.h"

class Batcher : public Transformable
{
  private:
	std::vector<Sprite*> sprites;

	bool blending = false;

  public:
	std::shared_ptr<GameMaterial> material;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	glm::vec2 texRes = {0, 0};

	bool batcherDirty = false;

  public:
	static std::shared_ptr<Batcher> create(std::string texture, bool blend = false);

	bool init(std::string texture, bool blend);

	void draw();

	void addSprite(Sprite* sp);

	void removeSprite(Sprite* sp);

	void sendVertex(Vertex&vert, size_t index);

	void updateSprite(Sprite* sp);

	void generateVertices();
};