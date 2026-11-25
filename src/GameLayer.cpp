#include "GameLayer.h"
#include "Application.h"
#include "Common.h"
#include "EffectGameObject.h"
#include "Level.h"
#include "config.h"

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

size_t LayerIndex(int zLayer)
{
	switch (zLayer)
	{
	case -3: return 0;
	case -1: return 1;
	case 1: return 2;
	case 3: return 3;
	case 7: return 5;
	case 9: return 6;
	default: return 4;
	}
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

	for (size_t layer = 0; layer < gameSheet01.size(); ++layer)
	{
		for (size_t blend = 0; blend < gameSheet01[layer].size(); ++blend)
		{
			const bool additive = blend == 1;
			gameSheet01[layer][blend] = Batcher::create("Resources/GJ_GameSheet-uhd.png", additive);
			gameSheet03[layer][blend] = Batcher::create("Resources/GJ_GameSheet03-uhd.png", additive);
			gameSheet04[layer][blend] = Batcher::create("Resources/GJ_GameSheet04-uhd.png", additive);
			if (!gameSheet01[layer][blend] || !gameSheet03[layer][blend] || !gameSheet04[layer][blend])
				return false;

			batchers.push_back(gameSheet01[layer][blend]);
			batchers.push_back(gameSheet03[layer][blend]);
			batchers.push_back(gameSheet04[layer][blend]);
		}
	}

	gameSheet02 = Batcher::create("Resources/GJ_GameSheet02-uhd.png");
	if (!gameSheet02)
		return false;
	batchers.push_back(gameSheet02);

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
	for (size_t layer = 0; layer < gameSheet01.size(); ++layer)
	{
		gameSheet03[layer][1]->draw();
		gameSheet04[layer][1]->draw();
		gameSheet01[layer][1]->draw();
		gameSheet03[layer][0]->draw();
		gameSheet04[layer][0]->draw();
		gameSheet01[layer][0]->draw();
	}
	gameSheet02->draw();
}

void GameLayer::drawBackground()
{
	scrolling.draw(camera, groundY, cameraY);
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
	for (size_t i = 0; i < colorChannels.size(); i++)
	{
		colorChannels[i] = ColorChannel::create({255, 255, 255, 255}, static_cast<int>(i));
	}

	for (size_t i = 0; i < groups.size(); i++)
	{
		groups[i] = Group::create();
	}

	setupLevel(levelStringView);
	setupObjects(levelStringView);

	if (objects.size() != 0)
	{
		std::sort(objects.begin(), objects.end(), [](const auto& x, const auto& y) { return x->zLayer < y->zLayer; });

		sectionObjects.resize(std::max(1, Common::sectionForPos(lastObjXPos)));

		for (std::shared_ptr<GameObject> object : objects)
		{
			if (object->section >= 0)
			{
				if (static_cast<size_t>(object->section) >= sectionObjects.size())
					sectionObjects.resize(object->section + 1);
				sectionObjects[object->section].push_back(object.get());
			}
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
				if (i < 0 || static_cast<size_t>(i) >= colorChannels[channel]->channelSprites.size())
					continue;
				for (Sprite* sprite : colorChannels[channel]->channelSprites[i])
				{
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
			if (i < 0 || static_cast<size_t>(i) >= groups[group]->objectsInSections.size())
				continue;
			for (GameObject* obj : groups[group]->objectsInSections[i])
				obj->updatePosition(true);
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
	const float zoom = std::max(camera.zoom, 0.01f);
	const float viewLeft = camera.target.x - camera.offset.x / zoom;
	const float viewRight = camera.target.x + (winSize.x - camera.offset.x) / zoom;
	const float prefetchDistance = 400.0f * screenScale / zoom;
	int prevSection = std::max(0, static_cast<int>(floorf((viewLeft - prefetchDistance) / 100.0f)));
	int nextSection = static_cast<int>(ceilf((viewRight + prefetchDistance) / 100.0f));

	int prevPrevSection = this->prevSection - 1;
	bool sectionInRange = prevPrevSection >= 0 &&
		static_cast<size_t>(prevPrevSection) < sectionObjects.size();

	if (sectionInRange && !sectionObjects.empty())
	{
		for (GameObject* obj : sectionObjects[prevPrevSection])
		{
			for (auto& spr : obj->childSprites)
				spr->removeFromBatcher();
			obj->removeFromBatcher();
		}
	}

	for (int i = prevSection; i < nextSection; i++)
	{
		if (i >= 0)
		{
			if (static_cast<size_t>(i) < sectionObjects.size())
			{
				for (GameObject* obj : sectionObjects[i])
				{
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
						if (hsv.size() >= 5)
						{
							hsvModifier.h = Common::stof(hsv[0]);
							hsvModifier.s = Common::stof(hsv[1]);
							hsvModifier.v = Common::stof(hsv[2]);
							hsvModifier.sChecked = Common::stoi(hsv[3]);
							hsvModifier.vChecked = Common::stoi(hsv[4]);
						}
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

	if (!objData.empty())
		objData.erase(objData.begin());

	if (!objData.empty())
	{
		const std::string_view& last = objData.back();
		if (last.size() < 2 || last[0] != '1' || last[1] != ',')
			objData.pop_back();
	}

	if (objData.empty())
		return;

	objects.reserve(objData.size());

	for (const std::string_view& objectDataSpecific : objData)
	{
		std::shared_ptr<GameObject> obj = GameObject::createFromString(objectDataSpecific);
		if (obj)
		{
			objects.push_back(obj);
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
	const size_t layer = LayerIndex(sprite->parent->zLayer);

	if (sprite->channel && sprite->channel->id == 1010)
	{
		bool hasSecondChannel = sprite->parent->secondaryColorChannel != -1;
		int channel = hasSecondChannel ? sprite->parent->secondaryColorChannel : sprite->parent->primaryColorChannel;
		if (channel >= 0 && static_cast<size_t>(channel) < colorChannels.size() && colorChannels[channel])
			blending = colorChannels[channel]->blending;
	}

	if (sprite->texDef->sheet == "GJ_GameSheet-uhd.png")
	{
		targetBatcher = gameSheet01[layer][blending].get();
	}
	else if (sprite->texDef->sheet == "GJ_GameSheet02-uhd.png")
	{
		targetBatcher = gameSheet02.get();
	}
	else if (sprite->texDef->sheet == "GJ_GameSheet03-uhd.png" ||
			 sprite->texDef->sheet == "GJ_GameSheet04-uhd.png")
	{
		auto& sheetBatchers = sprite->texDef->sheet == "GJ_GameSheet03-uhd.png"
			? gameSheet03
			: gameSheet04;
		targetBatcher = sheetBatchers[layer][blending].get();
	}

	glm::vec4 col = {255, 255, 255, 255};

	if (!targetBatcher)
		return;

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

	for (int i = prevSection; i <= curSection; i++)
	{
		if (i >= 0 && static_cast<size_t>(i) < sectionObjects.size())
		{
			auto& section = sectionObjects[i];

			for (GameObject* obj : section)
			{
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

		}
	}

	const int firstMirrorSection = std::max(0, prevSection);
	const int lastMirrorSection = std::min(curSection, static_cast<int>(sectionObjects.size()) - 1);
	for (int i = firstMirrorSection; i <= lastMirrorSection; ++i)
	{
		for (GameObject* obj : sectionObjects[i])
		{
			if ((obj->objectID == 45 || obj->objectID == 46) && obj->getPosition().x <= camPos &&
				!obj->mirrorPortalActivated)
			{
				Application::instance->mirrorX = !Application::instance->mirrorX;
				obj->mirrorPortalActivated = true;
			}
		}
	}
}
