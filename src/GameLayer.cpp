#include "GameLayer.h"
#include "Application.h"
#include "Common.h"
#include "EffectGameObject.h"
#include "Level.h"

#include <raymath.h>

#include <algorithm>
#include <deque>
#include <cmath>

GameLayer* GameLayer::instance;

namespace
{

Color ToRaylibColor(const glm::vec4& color)
{
	return {
		static_cast<unsigned char>(std::clamp(color.r, 0.0f, 255.0f)),
		static_cast<unsigned char>(std::clamp(color.g, 0.0f, 255.0f)),
		static_cast<unsigned char>(std::clamp(color.b, 0.0f, 255.0f)),
		static_cast<unsigned char>(std::clamp(color.a, 0.0f, 255.0f))
	};
}

bool IsSpeedPortal(std::string_view objectData)
{
	const auto properties = Common::splitByDelimStringView(objectData, ',');
	if (properties.size() < 2 || properties[0] != "1")
		return false;

	const std::string_view id = properties[1];
	return id == "200" || id == "201" || id == "202" || id == "203" || id == "1334";
}

}

std::shared_ptr<GameLayer> GameLayer::createWithLevelString(std::string_view lvlstr)
{
	std::shared_ptr<GameLayer> ptr(new GameLayer);

	if (ptr->initWithLevelString(lvlstr))
		return ptr;

	return nullptr;
}

bool GameLayer::initializeMembers()
{
	instance = this;
	Application::instance->mirrorX = false;

	gameSheet01_t3 = Batcher::create("Resources/GJ_GameSheet-uhd.png");
	if (!gameSheet01_t3)
		return false;

	gameSheet01_t3_blending = Batcher::create("Resources/GJ_GameSheet-uhd.png", true);
	gameSheet01_t2 = Batcher::create("Resources/GJ_GameSheet-uhd.png");
	gameSheet01_t2_blending = Batcher::create("Resources/GJ_GameSheet-uhd.png", true);
	gameSheet01_t1 = Batcher::create("Resources/GJ_GameSheet-uhd.png");
	gameSheet01_t1_blending = Batcher::create("Resources/GJ_GameSheet-uhd.png", true);
	gameSheet01_b4 = Batcher::create("Resources/GJ_GameSheet-uhd.png");
	gameSheet01_b4_blending = Batcher::create("Resources/GJ_GameSheet-uhd.png", true);
	gameSheet01_b3 = Batcher::create("Resources/GJ_GameSheet-uhd.png");
	gameSheet01_b3_blending = Batcher::create("Resources/GJ_GameSheet-uhd.png", true);
	gameSheet01_b2 = Batcher::create("Resources/GJ_GameSheet-uhd.png");
	gameSheet01_b2_blending = Batcher::create("Resources/GJ_GameSheet-uhd.png", true);
	gameSheet01_b1 = Batcher::create("Resources/GJ_GameSheet-uhd.png");
	gameSheet01_b1_blending = Batcher::create("Resources/GJ_GameSheet-uhd.png", true);
	gameSheet02 = Batcher::create("Resources/GJ_GameSheet02-uhd.png");
	if (!gameSheet02)
		return false;

	batchers = {gameSheet01_b1, gameSheet01_b1_blending, gameSheet01_b2, gameSheet01_b2_blending,
				gameSheet01_b3, gameSheet01_b3_blending, gameSheet01_b4, gameSheet01_b4_blending,
				gameSheet01_t3, gameSheet01_t3_blending, gameSheet01_t2, gameSheet01_t2_blending,
				gameSheet01_t1, gameSheet01_t1_blending, gameSheet02};

	updateLevelObjects();
	return true;
}

bool GameLayer::initWithLevelString(std::string_view levelString)
{
	if (!initializeMembers())
		return false;

	loadLevelFromLevelString(levelString);
	if (!scrolling.load(bgID))
		return false;

	scrolling.setBackgroundColor(ToRaylibColor(colorChannels[1000]->getColor()));
	scrolling.setGroundColor(ToRaylibColor(colorChannels[1001]->getColor()));

	return true;
}

void GameLayer::update()
{
	updateCamera();
	updateTriggers();

	processColorActions();

	updateLevelObjects();

	updateVisibility();
}

void GameLayer::draw()
{
	gameSheet01_b4_blending->draw();
	gameSheet01_b4->draw();
	gameSheet01_b3_blending->draw();
	gameSheet01_b3->draw();
	gameSheet01_b2_blending->draw();
	gameSheet01_b2->draw();
	gameSheet01_b1_blending->draw();
	gameSheet01_b1->draw();
	gameSheet01_t1_blending->draw();
	gameSheet01_t1->draw();
	gameSheet01_t2_blending->draw();
	gameSheet01_t2->draw();
	gameSheet01_t3_blending->draw();
	gameSheet01_t3->draw();
	gameSheet02->draw();
}

void GameLayer::drawBackground()
{
	scrolling.draw(camera);
}

void GameLayer::updateCamera()
{
	Application* app = Application::instance;

	float movementX = app->deltaTime * 311.58f;

	glm::vec2 prev = {camera.target.x, -camera.target.y};
	camera.target.x += movementX;

	deltaMove = glm::vec3(camera.target.x, -camera.target.y, 0) - glm::vec3(prev, 0);
	scrolling.update(movementX, camera.zoom);
}

void GameLayer::processColorActions()
{
	std::deque<std::shared_ptr<ColorAction>> toRemoveColor;
	std::deque<std::shared_ptr<CopyColorAction>> toRemoveCopyColor;
	std::deque<std::shared_ptr<PulseAction>> toRemovePulse;
	std::deque<std::shared_ptr<OpacityAction>> toRemoveOpacity;
	std::deque<std::shared_ptr<ActionInterval>> toRemoveMove;
	std::deque<std::shared_ptr<SpawnAction>> toRemoveSpawn;
	std::deque<std::shared_ptr<ActionInterval>> toRemoveRotate;
	std::deque<std::shared_ptr<FollowAction>> toRemoveFollow;

	for (auto& ac : colorActionsActive)
	{
		ac->step(Application::instance->deltaTime);

		if (ac->isDone())
			toRemoveColor.push_back(ac);
	}

	for (auto& ac : pulseActionsActive)
	{
		ac->step(Application::instance->deltaTime);

		if (ac->isDone())
			toRemovePulse.push_back(ac);
	}
	pulsedGroups.clear();

	for (auto& ac : opacityActionsActive)
	{
		ac->step(Application::instance->deltaTime);

		if (ac->isDone())
			toRemoveOpacity.push_back(ac);
	}

	for (auto& ac : copyColorActionsActive)
	{
		ac->step(Application::instance->deltaTime);

		if (ac->isDone())
			toRemoveCopyColor.push_back(ac);
	}

	for (auto& ac : moveActionsActive)
	{
		if (ac)
		{
			ac->step(Application::instance->deltaTime);

			if (ac->isDone())
				toRemoveMove.push_back(ac);
		}
	}

	for (auto& ac : spawnActionsActive)
	{
		ac->step(Application::instance->deltaTime);

		if (ac->isDone())
			toRemoveSpawn.push_back(ac);
	}

	for (auto& ac : rotateActionsActive)
	{
		ac->step(Application::instance->deltaTime);

		if (ac->isDone())
			toRemoveRotate.push_back(ac);
	}

	for (auto& ac : followActionsActive)
	{
		ac->step(Application::instance->deltaTime);

		if (ac->isDone())
			toRemoveFollow.push_back(ac);
	}

	for (auto& ac : toRemoveColor)
		colorActionsActive.erase(std::find(colorActionsActive.begin(), colorActionsActive.end(), ac));

	for (auto& ac : toRemovePulse)
		pulseActionsActive.erase(std::find(pulseActionsActive.begin(), pulseActionsActive.end(), ac));

	for (auto& ac : toRemoveOpacity)
		opacityActionsActive.erase(std::find(opacityActionsActive.begin(), opacityActionsActive.end(), ac));

	for (auto& ac : toRemoveCopyColor)
		copyColorActionsActive.erase(std::find(copyColorActionsActive.begin(), copyColorActionsActive.end(), ac));

	for (auto& ac : toRemoveMove)
		moveActionsActive.erase(std::find(moveActionsActive.begin(), moveActionsActive.end(), ac));

	for (auto& ac : toRemoveSpawn)
	{
		ac->spawn();
		spawnActionsActive.erase(std::find(spawnActionsActive.begin(), spawnActionsActive.end(), ac));
	}

	for (auto& ac : toRemoveRotate)
		rotateActionsActive.erase(std::find(rotateActionsActive.begin(), rotateActionsActive.end(), ac));

	for (auto& ac : toRemoveFollow)
		followActionsActive.erase(std::find(followActionsActive.begin(), followActionsActive.end(), ac));
}

void GameLayer::loadLevelFromLevelString(std::string_view levelStringView)
{
	for (int i = 0; i < 1013; i++)
	{
		colorChannels[i] = ColorChannel::create({255, 255, 255, 255}, i);
		groups[i] = Group::create();
	}

	setupLevel(levelStringView);
	setupObjects(levelStringView);

	if (objects.size() != 0)
	{
		std::sort(objects.begin(), objects.end(), [](const auto& x, const auto& y) { return x->zLayer < y->zLayer; });

		for (size_t i = 0; i < Common::sectionForPos(lastObjXPos); i++)
		{
			std::unordered_map<int, GameObject*> map;
			sectionObjects.push_back(map);
		}

		for (std::shared_ptr<GameObject> object : objects)
		{
			sectionObjects[object->section].insert({object->uniqueID, object.get()});
		}

		for (auto& section : sectionObjects)
		{
			if (section.size() > maxSectionSize)
				maxSectionSize = section.size();
		}
	}

	for (std::shared_ptr<ColorChannel>& channel : colorChannels)
		dirtyChannels.insert(channel->id);
}

void GameLayer::updateLevelObjects()
{
	for (int channel : dirtyChannels)
	{
		if (channel < 0)
			continue;

		switch (channel)
		{
		default:
			for (int i = prevSection; i < nextSection + 1; i++)
			{
				for (auto& pair : colorChannels[channel]->channelSprites[i])
				{
					Sprite* sprite = pair.second;
					sprite->setColorWithoutSend(colorChannels[channel]->getColor());
					sprite->setOpacity(colorChannels[channel]->getColor().a);
				}
			}
			break;
		case 1000:
			scrolling.setBackgroundColor(ToRaylibColor(colorChannels[channel]->getColor()));
			break;
		case 1001:
			scrolling.setGroundColor(ToRaylibColor(colorChannels[channel]->getColor()));
			break;
		}
	}
	dirtyChannels.clear();

	for (int group : dirtyGroups)
	{
		groups[group]->moveDelta = {0, 0};
		for (int i = prevSection; i < nextSection + 1; i++)
		{
			auto map = &groups[group]->objectsInSections[i];
			for (auto& pair : *map)
			{
				GameObject* obj = pair.second;
				obj->updatePosition(true);
			}
		}
	}
	dirtyGroups.clear();
}

void GameLayer::exit()
{
	scrolling.unload();
	if (instance == this)
		instance = nullptr;
}

void GameLayer::updateVisibility()
{
	auto& winSize = Application::instance->resolution;
	const float viewLeft = camera.target.x - camera.offset.x / camera.zoom;
	const float viewRight = camera.target.x + (winSize.x - camera.offset.x) / camera.zoom;
	const float prefetchDistance = 100.0f;
	int prevSection = std::max(0, static_cast<int>(floorf((viewLeft - prefetchDistance) / 100.0f)));
	int nextSection = static_cast<int>(ceilf((viewRight + prefetchDistance) / 100.0f));

	int prevPrevSection = this->prevSection - 1;
	bool sectionInRange = prevPrevSection >= 0 && prevPrevSection < sectionObjects.size();

	if (sectionInRange && !sectionObjects.empty())
	{
		for (auto& pair : sectionObjects[prevPrevSection])
		{
			GameObject* obj = pair.second;
			for (auto& spr : obj->childSprites)
				spr->removeFromBatcher();
			obj->removeFromBatcher();
		}
	}

	for (int i = prevSection; i < nextSection; i++)
	{
		if (i >= 0)
		{
			if (i < sectionObjects.size())
			{
				auto section = &sectionObjects[i];
				for (auto& pair : *section)
				{
					GameObject* obj = pair.second;
					if (!obj || (obj->currentBatcher != nullptr) || !obj->enabled)
						continue;

					if (obj->isTrigger)
						continue;

					for (std::shared_ptr<Sprite>& sprite : obj->childSprites)
						layerObject(sprite.get());

					layerObject(obj);

					obj->updatePosition(false);
					obj->updateOpacity();
				}
			}
		}
	}

	for (std::shared_ptr<Batcher> batcher : batchers)
	{
		if (batcher->batcherDirty)
			batcher->generateVertices();
	}

	this->prevSection = prevSection;
	this->nextSection = nextSection;
}

void GameLayer::setupLevel(std::string_view levelString)
{
	std::vector<std::string_view> levelData =
		Common::splitByDelimStringView(Common::splitByDelimStringView(levelString, ';')[0], ',');

	for (size_t i = 0; i < levelData.size() - 1; i += 2)
	{
		if (levelData[i] == "kS1")
		{
			colorChannels[1000]->setR(Common::stoi(levelData[i + 1]));
		}
		else if (levelData[i] == "kS2")
		{
			colorChannels[1000]->setG(Common::stoi(levelData[i + 1]));
		}
		else if (levelData[i] == "kS3")
		{
			colorChannels[1000]->setB(Common::stoi(levelData[i + 1]));
		}
		else if (levelData[i] == "kS4")
		{
			colorChannels[1001]->setR(Common::stoi(levelData[i + 1]));
		}
		else if (levelData[i] == "kS5")
		{
			colorChannels[1001]->setG(Common::stoi(levelData[i + 1]));
		}
		else if (levelData[i] == "kS6")
		{
			colorChannels[1001]->setB(Common::stoi(levelData[i + 1]));
		}
		else if (levelData[i] == "kS29")
		{
			std::vector<std::string_view> colorString = Common::splitByDelimStringView(levelData[i + 1], '_');
			fillColorChannel(colorString, 1000);
		}
		else if (levelData[i] == "kS30")
		{
			std::vector<std::string_view> colorString = Common::splitByDelimStringView(levelData[i + 1], '_');
			fillColorChannel(colorString, 1001);
		}
		else if (levelData[i] == "kS31")
		{
			std::vector<std::string_view> colorString = Common::splitByDelimStringView(levelData[i + 1], '_');
			fillColorChannel(colorString, 1002);
		}
		else if (levelData[i] == "kS32")
		{
			std::vector<std::string_view> colorString = Common::splitByDelimStringView(levelData[i + 1], '_');
			fillColorChannel(colorString, 1004);
		}
		else if (levelData[i] == "kS37")
		{
			std::vector<std::string_view> colorString = Common::splitByDelimStringView(levelData[i + 1], '_');
			fillColorChannel(colorString, 1003);
		}
		else if (levelData[i] == "kS38")
		{
			std::vector<std::string_view> colorString = Common::splitByDelimStringView(levelData[i + 1], '|');
			for (std::string_view colorData : colorString)
			{
				std::vector<std::string_view> innerData = Common::splitByDelimStringView(colorData, '_');

				int key = 0;
				glm::vec4 col;
				bool blending = false;
				HSV hsvModifier;
				int copyID = -5;

				for (size_t j = 0; j < innerData.size() - 1; j += 2)
				{
					switch (Common::stoi(innerData[j]))
					{
					case 1:
						col.r = Common::stoi(innerData[j + 1]);
						break;
					case 2:
						col.g = Common::stoi(innerData[j + 1]);
						break;
					case 3:
						col.b = Common::stoi(innerData[j + 1]);
						break;
					case 5:
						blending = Common::stoi(innerData[j + 1]);
						break;
					case 6:
						key = Common::stoi(innerData[j + 1]);
						break;
					case 7:
						col.a = Common::stof(innerData[j + 1]) * 255.f;
						break;
					case 9: {
						copyID = Common::stoi(innerData[j + 1]);
					}
					break;
					case 10:
						std::vector<std::string_view> hsv = Common::splitByDelimStringView(innerData[j + 1], 'a');
						hsvModifier.h = Common::stof(hsv[0]);
						hsvModifier.s = Common::stof(hsv[1]);
						hsvModifier.v = Common::stof(hsv[2]);
						hsvModifier.sChecked = Common::stoi(hsv[3]);
						hsvModifier.vChecked = Common::stoi(hsv[4]);
						break;
					}
				}

				std::shared_ptr<ColorChannel> channel = colorChannels[key];
				channel->setColor(col);
				channel->blending = blending;
				channel->hsvModifier = hsvModifier;
				if (copyID != -5)
				{
					channel->copyColor = colorChannels[copyID].get();
					colorChannels[copyID]->copiers.push_back(channel.get());
				}
			}
		}
		else if (levelData[i] == "kA6")
		{
			bgID = Common::stoi(levelData[i + 1]);
			if (!bgID)
				bgID = 1;
		}
	}

	colorChannels[1005]->setColor(glm::vec4(0, 0, 255, 255));
	colorChannels[1005]->blending = true;
	colorChannels[1006]->setColor(glm::vec4(255, 255, 0, 255));
	colorChannels[1006]->blending = true;
	colorChannels[1010]->setColor(glm::vec4(0, 0, 0, 255));
	colorChannels[1005]->setColor(colorChannels[1000]->getColor());
	colorChannels[1007]->blending = true;
}

void GameLayer::setupObjects(std::string_view levelString)
{
	std::vector<std::string_view> objData = Common::splitByDelimStringView(levelString, ';');

	objData.erase(objData.begin());

	if (const std::string_view& last = objData.back(); last.front() != '1' || last[1] != ',')
		objData.pop_back();

	objects.reserve(objData.size());

	for (const std::string_view& objectDataSpecific : objData)
	{
		if (IsSpeedPortal(objectDataSpecific))
			continue;

		std::shared_ptr<GameObject> obj = GameObject::createFromString(objectDataSpecific);
		if (obj)
		{
			objects.push_back(obj);
			obj->objectIndex = GameLayer::instance->objects.size() - 1;

			if (obj->getPosition().x > lastObjXPos)
				lastObjXPos = obj->getPosition().x;
		}
	}
}

void GameLayer::layerObject(Sprite* sprite)
{
	if (sprite->currentBatcher)
		return;

	Batcher* targetBatcher = nullptr;
	bool blending = sprite->channel && sprite->channel->blending;

	if (sprite->channel && sprite->channel->id == 1010)
	{
		bool hasSecondChannel = sprite->parent->secondaryColorChannel != -1;
		int channel = hasSecondChannel ? sprite->parent->secondaryColorChannel : sprite->parent->primaryColorChannel;
		blending = colorChannels[channel]->blending;
	}

	if (sprite->texDef->sheet == "GJ_GameSheet-uhd.png")
	{
		switch (sprite->parent->zLayer)
		{
		case -3:
			targetBatcher = blending ? gameSheet01_b4_blending.get() : gameSheet01_b4.get();
			break;
		case -1:
			targetBatcher = blending ? gameSheet01_b3_blending.get() : gameSheet01_b3.get();
			break;
		case 1:
			targetBatcher = blending ? gameSheet01_b2_blending.get() : gameSheet01_b2.get();
			break;
		case 3:
			targetBatcher = blending ? gameSheet01_b1_blending.get() : gameSheet01_b1.get();
			break;
		default:
		case 5:
			targetBatcher = blending ? gameSheet01_t1_blending.get() : gameSheet01_t1.get();
			break;
		case 7:
			targetBatcher = blending ? gameSheet01_t2_blending.get() : gameSheet01_t2.get();
			break;
		case 9:
			targetBatcher = blending ? gameSheet01_t3_blending.get() : gameSheet01_t3.get();
			break;
		}
	}
	else if (sprite->texDef->sheet == "GJ_GameSheet02-uhd.png")
	{
		targetBatcher = gameSheet02.get();
	}

	glm::vec4 col = {255, 255, 255, 255};

	targetBatcher->addSprite(sprite);

	if (sprite->channel)
	{
		col = sprite->channel->getColor();
	}

	sprite->setColorWithoutSend({col.r, col.g, col.b, 255});
	sprite->setOpacityWithoutSend(col.a);

	sprite->updateVerticesPosition(false);
}

void GameLayer::fillColorChannel(std::span<std::string_view> colorString, int id)
{
	for (size_t j = 0; j < colorString.size() - 1; j += 2)
	{
		switch (Common::stoi(colorString[j]))
		{
		case 1:
			colorChannels[id]->setR(Common::stoi(colorString[j + 1]));
			break;
		case 2:
			colorChannels[id]->setG(Common::stoi(colorString[j + 1]));
			break;
		case 3:
			colorChannels[id]->setB(Common::stoi(colorString[j + 1]));
			break;
		}
	}
}

void GameLayer::updateTriggers()
{
	float camPos = camera.target.x;

	int curSection = Common::sectionForPos(camPos);

	for (int i = prevSection; i < curSection; i++)
	{
		if (i < sectionObjects.size() && i >= 0)
		{
			auto& section = sectionObjects[i];

			for (auto& pair : section)
			{
				GameObject* obj = pair.second;
				if (obj->getPosition().x > camPos)
					continue;

				if (obj->isTrigger)
				{
					EffectGameObject* tr = static_cast<EffectGameObject*>(obj);
					if (tr->enabled)
					{
						if (!tr->spawnTriggered && !tr->touchTriggered && !tr->triedToActivate)
							tr->triggerActivated();
					}
					else
						tr->triedToActivate = true;
				}
			}

			if (curSection >= 0 && curSection < static_cast<int>(sectionObjects.size()))
			{
				for (auto& pair : sectionObjects[curSection])
				{
					GameObject* obj = pair.second;
					if ((obj->objectID == 45 || obj->objectID == 46) && obj->getPosition().x <= camPos &&
						!obj->mirrorPortalActivated)
					{
						Application::instance->mirrorX = !Application::instance->mirrorX;
						obj->mirrorPortalActivated = true;
					}
				}
			}
		}
	}
}
