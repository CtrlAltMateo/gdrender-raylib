#include "Sprite.h"
#include <algorithm>
#include <glm/vec2.hpp>

#include "Batcher.h"
#include "ColorChannel.h"
#include "Common.h"
#include "GameLayer.h"
#include "GameObject.h"

void Sprite::updateVerticesPosition(bool send)
{
	if (currentBatcher == nullptr)
		return;

	if (isDirty())
		updateTransform();

	glm::mat4 combinedTransform = getTransform();

	if (parent && parent != this)
	{
		if (parent->isDirty())
			parent->updateTransform();

		combinedTransform = parent->getTransform() * getTransform();
	}

	if (send)
	{
		for (int i = 0; i < 4; ++i)
		{
			updateVertexPosition(i, combinedTransform);
			sendVertex(i);
		}
	}
	else
	{
		for (int i = 0; i < 4; ++i)
			updateVertexPosition(i, combinedTransform);
	}

	/* if (send)
		currentBatcher->updateSprite(this); */
}

void Sprite::sendVertices()
{
	if (currentBatcher == nullptr)
		return;

	for (int i = 0; i < 4; ++i)
		sendVertex(i);

	// currentBatcher->updateSprite(this);
}

void Sprite::updateVertexPosition(int index, glm::mat4& tr)
{
	Vertex& vertex = vertices[index];

	glm::vec2& pos = originalPositions[index];
	vertex.position = {pos.x * half.x + anchor.x, pos.y * half.y + anchor.y};

	vertex.position = Common::transformPoint(vertex.position, tr);
	vertex.position.y *= -1;
}

void Sprite::sendVertex(int index)
{
	size_t atIndex = atlasIndex * 4;
	// TODO: temporary solution, find out what's causing this
	if (atIndex >= currentBatcher->vertices.size())
		return;

	currentBatcher->sendVertex(vertices[index], atIndex + index);
}

std::shared_ptr<Sprite> Sprite::create(std::string texture)
{
	std::shared_ptr<Sprite> ptr = std::make_shared<Sprite>();

	if (ptr->init(texture))
		return ptr;

	return nullptr;
}

void Sprite::setColor(glm::vec4 color)
{
	setColorWithoutSend(color);
	sendVertices();
}

void Sprite::setColorWithoutSend(glm::vec4 color)
{
	hsvAppliedColor = hsvModifier ? HSV::combine(color, *hsvModifier) : color;
	for (int i = 0; i < 4; i++)
	{
		vertices[i].color.r = hsvAppliedColor.r;
		vertices[i].color.g = hsvAppliedColor.g;
		vertices[i].color.b = hsvAppliedColor.b;
	}
}

void Sprite::setOpacity(float opacity)
{
	setOpacityWithoutSend(opacity);
	sendVertices();
}

void Sprite::setOpacityWithoutSend(float opacity)
{
	/* if (this->channel && this->channel->blending)
	{
		double transformed_opacity = std::clamp(
			(0.175656971639325 * std::pow(7.06033051530761, static_cast<double>(opacity / 255.f)) - 0.213355914301931),
			0.0, 1.0);
		opacity = static_cast<float>(transformed_opacity) * 255.f;
	} */
	this->opacity = opacity;
	for (int i = 0; i < 4; i++)
	{
		vertices[i].color.a = opacity * opacityMultiplier;
	}
}

void Sprite::removeFromBatcher()
{
	if (currentBatcher)
		currentBatcher->removeSprite(this);
}

bool Sprite::init(std::string texture)
{
	if (!ImageLoader::images.count(texture))
		return false;

	texDef = &ImageLoader::images[texture];

	float z = texDef->textureRect.z / TextureScale::UhdPixelsPerGameUnit;
	float w = texDef->textureRect.w / TextureScale::UhdPixelsPerGameUnit;

	half = {z / 2, w / 2};

	if (texDef->rotated)
		half = {half.y, half.x};

	anchor = texDef->anchor;

	if (texDef->rotated)
		anchor = {-anchor.y, anchor.x};

	anchor.x *= -1;
	anchor.y *= -1;

	vertices[0].position = glm::vec2(-1, -1);
	vertices[1].position = glm::vec2(1, -1);
	vertices[2].position = glm::vec2(1, 1);
	vertices[3].position = glm::vec2(-1, 1);
	originalPositions[0] = vertices[0].position;
	originalPositions[1] = vertices[1].position;
	originalPositions[2] = vertices[2].position;
	originalPositions[3] = vertices[3].position;

	const float texLeft = texDef->textureRect.x + 0.5f;
	const float texRight = texDef->textureRect.x + texDef->textureRect.z - 0.5f;
	const float texTop = texDef->textureRect.y + 0.5f;
	const float texBottom = texDef->textureRect.y + texDef->textureRect.w - 0.5f;

	if (!texDef->rotated)
	{
		vertices[0].texCoord = glm::vec2(texLeft, texBottom);
		vertices[1].texCoord = glm::vec2(texRight, texBottom);
		vertices[2].texCoord = glm::vec2(texRight, texTop);
		vertices[3].texCoord = glm::vec2(texLeft, texTop);
	}
	else
	{
		vertices[0].texCoord = glm::vec2(texLeft, texTop);
		vertices[1].texCoord = glm::vec2(texLeft, texBottom);
		vertices[2].texCoord = glm::vec2(texRight, texBottom);
		vertices[3].texCoord = glm::vec2(texRight, texTop);
	}

	return true;
}

void Sprite::removeFromChannel()
{
	if (!channel || !parent || parent->section < 0 ||
		static_cast<size_t>(parent->section) >= channel->channelSprites.size())
		return;

	auto& sprites = channel->channelSprites[parent->section];
	std::erase(sprites, this);
}

void Sprite::addToChannelSection()
{
	if (!channel || !parent || parent->section < 0)
		return;

	if (static_cast<size_t>(parent->section) >= channel->channelSprites.size())
		channel->channelSprites.resize(parent->section + 1);
	auto& sprites = channel->channelSprites[parent->section];
	if (std::find(sprites.begin(), sprites.end(), this) == sprites.end())
		sprites.push_back(this);
}
