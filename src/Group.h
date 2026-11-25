#pragma once

#include <vector>
#include <memory>
#include <glm/vec2.hpp>

class EffectGameObject;
class GameObject;
class ActionInterval;

class Group
{
public:
	std::vector<std::vector<GameObject*>> objectsInSections;
	std::vector<GameObject*> objects;
	float groupOpacity = 1.0f;

	glm::vec2 moveDelta = {0, 0};

	std::vector<EffectGameObject*> spawnTriggered;

	std::shared_ptr <ActionInterval> rotateAction = nullptr;

	bool enabled = true;
public:
	static std::shared_ptr<Group> create();
};