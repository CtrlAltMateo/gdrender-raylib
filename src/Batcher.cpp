#include "Batcher.h"
#include "GameObject.h"

#include <algorithm>
#include <rlgl.h>

std::shared_ptr<Batcher> Batcher::create(std::string texture, bool blend)
{
	std::shared_ptr<Batcher> ptr = std::make_shared<Batcher>();

	if (ptr->init(texture, blend))
		return ptr;

	return nullptr;
}

void Batcher::draw()
{
	if (!material || material->getTextures().empty() || indices.empty())
		return;

	BeginBlendMode(blending ? BLEND_ADDITIVE : BLEND_ALPHA);
	rlSetTexture(material->getTextures().front()->getTexture().id);
	rlBegin(RL_TRIANGLES);
	for (uint32_t index : indices)
	{
		const Vertex& vertex = vertices[index];
		rlColor4ub(
			static_cast<unsigned char>(std::clamp(vertex.color.r, 0.0f, 255.0f)),
			static_cast<unsigned char>(std::clamp(vertex.color.g, 0.0f, 255.0f)),
			static_cast<unsigned char>(std::clamp(vertex.color.b, 0.0f, 255.0f)),
			static_cast<unsigned char>(std::clamp(vertex.color.a, 0.0f, 255.0f)));
		rlTexCoord2f(vertex.texCoord.x / texRes.x, vertex.texCoord.y / texRes.y);
		rlVertex2f(vertex.position.x, vertex.position.y);
	}
	rlEnd();
	rlSetTexture(0);
	EndBlendMode();

	needSend = false;
	minRange = std::numeric_limits<size_t>::max();
	maxRange = 0;
}

void Batcher::sendVertex(Vertex& vert, size_t index)
{
	vertices[index] = vert;
	needSend = true;

	minRange = std::min(minRange, index);
	maxRange = std::max(maxRange, index);
}

void Batcher::updateSprite(Sprite* sp)
{
	if (sp->currentBatcher != this)
		return;

	size_t atIndex = sp->atlasIndex * 4;
	for (int i = 0; i < 4; i++)
		vertices[atIndex + i] = sp->vertices[i];

}

bool Batcher::init(std::string texture, bool blend)
{
	this->blending = blend;
	material = GameMaterial::create();

	auto tex = GameTexture::create(texture);
	if (!material || !tex)
		return false;
	texRes = tex->resolution;
	material->addTexture(tex, 0);

	vertices.clear();
	indices.clear();

	return true;
}

void Batcher::addSprite(Sprite* sp)
{
	if (sp->currentBatcher != nullptr)
		return;

	sprites.insert({sp->uniqueID, sp});

	batcherDirty = true;

	sp->currentBatcher = this;
}

void Batcher::removeSprite(Sprite* sp)
{
	if (batcherDirty)
		return;

	sp->setOpacity(0);

	sprites.unordered_erase(sp->uniqueID);

	sp->currentBatcher = nullptr;
}

void Batcher::generateVertices()
{
	vertices.clear();
	indices.clear();

	auto data = sprites.values_container();

	std::sort(data.begin(), data.end(), [](const auto& x, const auto& y) {
		if (!x.second->parent || !y.second->parent)
			return false;

		return x.second->parent->zOrder < y.second->parent->zOrder;
	});

	size_t i = 0;
	size_t triangleIndex = 0;

	for (auto& pair : data)
	{
		Sprite* sprite = pair.second;
		sprite->atlasIndex = i;
		vertices.push_back(sprite->vertices[0]);
		vertices.push_back(sprite->vertices[1]);
		vertices.push_back(sprite->vertices[2]);
		vertices.push_back(sprite->vertices[3]);

		indices.push_back(static_cast<uint32_t>(triangleIndex));
		indices.push_back(static_cast<uint32_t>(triangleIndex + 1));
		indices.push_back(static_cast<uint32_t>(triangleIndex + 2));

		indices.push_back(static_cast<uint32_t>(triangleIndex + 2));
		indices.push_back(static_cast<uint32_t>(triangleIndex + 3));
		indices.push_back(static_cast<uint32_t>(triangleIndex));

		triangleIndex += 4;
		i++;
	}

	needSend = false;

	minRange = std::numeric_limits<size_t>::max();
	maxRange = 0;

	batcherDirty = false;
}
