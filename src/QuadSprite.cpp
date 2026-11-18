#include "QuadSprite.h"

#include "Common.h"

#include <algorithm>
#include <rlgl.h>

std::shared_ptr<QuadSprite> QuadSprite::create()
{
	std::shared_ptr<QuadSprite> ptr = std::make_shared<QuadSprite>();

	if (ptr->init())
		return ptr;

	return nullptr;
}

void QuadSprite::draw()
{
	if (!material || material->getTextures().empty())
		return;

	Texture2D texture = material->getTextures().front()->getTexture();
	if (isDirty())
		updateTransform();
	glm::mat4 transform = getTransform();

	BeginBlendMode(BLEND_ALPHA);
	rlSetTexture(texture.id);
	rlBegin(RL_TRIANGLES);
	for (uint32_t index : indices)
	{
		const Vertex& vertex = vertices[index];
		rlColor4ub(
			static_cast<unsigned char>(std::clamp(vertex.color.r, 0.0f, 255.0f)),
			static_cast<unsigned char>(std::clamp(vertex.color.g, 0.0f, 255.0f)),
			static_cast<unsigned char>(std::clamp(vertex.color.b, 0.0f, 255.0f)),
			static_cast<unsigned char>(std::clamp(vertex.color.a, 0.0f, 255.0f)));
		rlTexCoord2f(vertex.texCoord.x, vertex.texCoord.y);
		glm::vec2 position = Common::transformPoint(vertex.position, transform);
		rlVertex2f(position.x, -position.y);
	}
	rlEnd();
	rlSetTexture(0);
	EndBlendMode();
}

void QuadSprite::setColor(glm::vec4 color)
{
	for(auto&vert : vertices)
		vert.color = color;

}

bool QuadSprite::init()
{
	material = GameMaterial::create();

	vertices[0].position = glm::vec2(-1, -1);
	vertices[1].position = glm::vec2(1, -1);
	vertices[2].position = glm::vec2(1, 1);
	vertices[3].position = glm::vec2(-1, 1);
	vertices[0].texCoord = glm::vec2(0, 0);
	vertices[1].texCoord = glm::vec2(1, 0);
	vertices[2].texCoord = glm::vec2(1, 1);
	vertices[3].texCoord = glm::vec2(0, 1);

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;
    indices[3] = 2;
    indices[4] = 3;
    indices[5] = 0;

	return true;
}