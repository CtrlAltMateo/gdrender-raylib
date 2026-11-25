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
	rlDrawRenderBatchActive();
	rlDisableBackfaceCulling();
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
	rlDrawRenderBatchActive();
	rlEnableBackfaceCulling();
	rlSetTexture(0);
	EndBlendMode();
}

void Batcher::sendVertex(Vertex& vert, size_t index)
{
	if (index >= vertices.size())
		return;
	vertices[index] = vert;
}

void Batcher::updateSprite(Sprite* sp)
{
	if (sp->currentBatcher != this)
		return;

	const size_t atIndex = sp->atlasIndex * 4;
	if (atIndex + 3 >= vertices.size())
		return;
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
	if (!sp || sp->currentBatcher != nullptr)
		return;

	sprites.push_back(sp);
	batcherDirty = true;
	sp->currentBatcher = this;
}

void Batcher::removeSprite(Sprite* sp)
{
	if (!sp || sp->currentBatcher != this)
		return;

	const auto sprite = std::find(sprites.begin(), sprites.end(), sp);
	if (sprite != sprites.end())
		sprites.erase(sprite);
	sp->setOpacityWithoutSend(0);
	sp->currentBatcher = nullptr;
	batcherDirty = true;
}

void Batcher::generateVertices()
{
	vertices.clear();
	indices.clear();
	std::stable_sort(sprites.begin(), sprites.end(), [](const Sprite* left, const Sprite* right) {
		const int leftZ = left->parent ? left->parent->zOrder : 0;
		const int rightZ = right->parent ? right->parent->zOrder : 0;
		return leftZ < rightZ;
	});

	size_t i = 0;
	size_t triangleIndex = 0;

	for (Sprite* sprite : sprites)
	{
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

	batcherDirty = false;
}
