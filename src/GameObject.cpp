#include "GameObject.h"
#include "Common.h"
#include "EffectGameObject.h"
#include "GameLayer.h"
#include "ImageLoader.h"
#include "LongData.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

nlohmann::json GameObject::objJson;

namespace
{

ColorChannel* ResolveColorChannel(int channelId, int fallbackId)
{
	const auto& channels = GameLayer::instance->colorChannels;
	if (channelId >= 0 && static_cast<size_t>(channelId) < channels.size() && channels[channelId])
		return channels[channelId].get();
	if (fallbackId >= 0 && static_cast<size_t>(fallbackId) < channels.size() && channels[fallbackId])
		return channels[fallbackId].get();
	return channels[1010].get();
}

}

void GameObject::setupCustomObjects(const nlohmann::json& objectJson, std::shared_ptr<GameObject> parent)
{
	const auto children = objectJson.find("children");
	if (children == objectJson.end() || !children->is_array())
		return;

	for (const nlohmann::json& sprite : *children)
	{
		std::shared_ptr<Sprite> spr = Sprite::create(sprite["texture"]);
		if (!spr)
			continue;

		spr->parent = this;

		glm::vec2 flip = {sprite["flip_x"] ? -1.f : 1.f, sprite["flip_y"] ? -1.f : 1.f};

		spr->setScale({sprite["scale_x"].get<float>() * flip.x, sprite["scale_y"].get<float>() * flip.y, 1});
		spr->setPosition({sprite["x"].get<float>(), sprite["y"].get<float>(), 0});
		spr->setRotation({0, 0, sprite["rot"].get<float>()});

		float width = spr->texDef->textureRect.z / TextureScale::UhdPixelsPerGameUnit;
		float height = spr->texDef->textureRect.w / TextureScale::UhdPixelsPerGameUnit;

		glm::vec2 _anchor = {width * sprite["anchor_x"].get<float>() * flip.x,
							 height * sprite["anchor_y"].get<float>() * flip.y};

		if (spr->texDef->rotated)
			_anchor = {-_anchor.y, _anchor.x};

		spr->anchor -= _anchor;

		if (sprite.contains("color_type"))
		{
			if (sprite["color_type"] == "Base" || objectJson.contains("swap_base_detail") &&
													  sprite["color_type"] == "Detail" &&
													  objectJson["swap_base_detail"])
			{
				spr->channel = ResolveColorChannel(primaryColorChannel, 1010);
				spr->hsvModifier = &primaryHSV;
				spr->channelType = 1;
			}
			else if (sprite["color_type"] == "Detail")
			{
				spr->channel = ResolveColorChannel(secondaryColorChannel, primaryColorChannel);
				spr->hsvModifier = &secondaryHSV;
				spr->channelType = 2;
			}
			else
			{
				spr->channel = ResolveColorChannel(1010, 1010);
				spr->channelType = 0;
			}
		}

		childSprites.push_back(spr);
		spr->childIndex = childSprites.size() - 1;

		spr->addToChannelSection();

		setupCustomObjects(sprite, parent);
	}
}

std::shared_ptr<GameObject> GameObject::createFromString(std::string_view str)
{
	if (objJson.empty())
	{
		std::ifstream file(std::filesystem::path("CustomResources") / "object.json");
		objJson = nlohmann::json::parse(file);
	}

	std::vector<std::string_view> properties = Common::splitByDelimStringView(str, ',');

	if (properties.size() <= 1)
		return nullptr;

	int objID = Common::stoi(properties[1]);

	if (!objJson.contains(properties[1]))
	{
		return nullptr;
	}

	const nlohmann::json& objectEntry = objJson[properties[1]];

	std::string texture = "emptyFrame.png";

	if (objectEntry.contains("texture"))
		texture = objectEntry["texture"];

	std::shared_ptr<GameObject> ptr;
	std::shared_ptr<EffectGameObject> effectPtr;

	if (std::find(LongData::triggers.begin(), LongData::triggers.end(), objID) != LongData::triggers.end())
	{
		effectPtr = std::shared_ptr<EffectGameObject>(new EffectGameObject);
		ptr = effectPtr;
		ptr->isTrigger = true;
	}
	else
	{
		ptr = std::shared_ptr<GameObject>(new GameObject);
	}

	if (!ptr || !ptr->init(texture))
		return nullptr;

	ptr->objectID = objID;

	if (objectEntry.contains("default_z_layer"))
		ptr->zLayer = objectEntry["default_z_layer"];
	if (objectEntry.contains("default_base_color_channel"))
		ptr->primaryColorChannel = objectEntry["default_base_color_channel"];
	if (objectEntry.contains("default_detail_color_channel"))
		ptr->secondaryColorChannel = objectEntry["default_detail_color_channel"];

	if (objectEntry.contains("default_z_order"))
		ptr->zOrder = objectEntry["default_z_order"];

	for (size_t i = 0; i < properties.size() - 1; i += 2)
	{
		int key = Common::stoi(properties[i]);

		if (!effectPtr)
		{
			switch (key)
			{
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 17:
			case 23:
			case 28:
			case 29:
			case 30:
			case 35:
			case 45:
			case 46:
			case 47:
			case 48:
			case 49:
			case 50:
			case 52:
			case 58:
			case 59:
			case 62:
			case 63:
			case 65:
			case 66:
			case 68:
			case 69:
			case 70:
			case 71:
			case 72:
			case 73:
			case 85:
			case 87:
				continue;
			}
		}

		switch (key)
		{
		case 2:
			ptr->setPosition({Common::stof(properties[i + 1]), ptr->getPosition().y, 0});
			ptr->section = Common::sectionForPos(ptr->getPosition().x);
			ptr->section = ptr->section - 1 < 0 ? 0 : ptr->section - 1;
			break;
		case 3:
			ptr->setPosition({ptr->getPosition().x, Common::stof(properties[i + 1]), 0});
			break;
		case 4:
			ptr->setScale({-1.f * Common::stof(properties[i + 1]), 1.f, 1});
			break;
		case 5:
			ptr->setScale({ptr->getScale().x, -1.f * Common::stof(properties[i + 1]), 1});
			break;
		case 6:
			ptr->setRotation({0, 0, -Common::stof(properties[i + 1])});
			break;
		case 7:
			effectPtr->triggerColor.r = Common::stoi(properties[i + 1]);
			break;
		case 8:
			effectPtr->triggerColor.g = Common::stoi(properties[i + 1]);
			break;
		case 9:
			effectPtr->triggerColor.b = Common::stoi(properties[i + 1]);
			break;
		case 10:
			effectPtr->duration = Common::stof(properties[i + 1]);
			if (effectPtr->duration < 0)
				effectPtr->duration = 0;
			break;
		case 11:
			effectPtr->touchTriggered = Common::stoi(properties[i + 1]);
			break;
		case 17:
			effectPtr->blending = Common::stoi(properties[i + 1]);
			break;
		case 21:
			ptr->primaryColorChannel = Common::stoi(properties[i + 1]);
			break;
		case 22:
			ptr->secondaryColorChannel = Common::stoi(properties[i + 1]);
			break;
		case 23:
			effectPtr->targetColorId = Common::stoi(properties[i + 1]);
			break;
		case 24:
			ptr->zLayer = Common::stoi(properties[i + 1]);
			break;
		case 25:
			ptr->zOrder = Common::stoi(properties[i + 1]);
			break;
		case 28:
			effectPtr->movement.x = Common::stoi(properties[i + 1]);
			break;
		case 29:
			effectPtr->movement.y = Common::stoi(properties[i + 1]);
			break;
		case 30:
			effectPtr->easing = Common::stoi(properties[i + 1]);
			break;
		case 32:
			ptr->setScale({ptr->getScale().x * Common::stof(properties[i + 1]),
						   ptr->getScale().y * Common::stof(properties[i + 1]), 1});
			break;
		case 35:
			effectPtr->triggerColor.a = Common::stof(properties[i + 1]) * 255.f;
			break;
		case 43: {
			std::vector<std::string_view> hsv = Common::splitByDelimStringView(properties[i + 1], 'a');
			if (hsv.size() >= 5)
			{
				ptr->primaryHSV.h = Common::stof(hsv[0]);
				ptr->primaryHSV.s = Common::stof(hsv[1]);
				ptr->primaryHSV.v = Common::stof(hsv[2]);
				ptr->primaryHSV.sChecked = Common::stoi(hsv[3]);
				ptr->primaryHSV.vChecked = Common::stoi(hsv[4]);
			}
		}
		break;
		case 44: {
			std::vector<std::string_view> hsv = Common::splitByDelimStringView(properties[i + 1], 'a');
			if (hsv.size() >= 5)
			{
				ptr->secondaryHSV.h = Common::stof(hsv[0]);
				ptr->secondaryHSV.s = Common::stof(hsv[1]);
				ptr->secondaryHSV.v = Common::stof(hsv[2]);
				ptr->secondaryHSV.sChecked = Common::stoi(hsv[3]);
				ptr->secondaryHSV.vChecked = Common::stoi(hsv[4]);
			}
		}
		break;
		case 45:
			effectPtr->fadeIn = Common::stof(properties[i + 1]);
			break;
		case 46:
			effectPtr->hold = Common::stof(properties[i + 1]);
			break;
		case 47:
			effectPtr->fadeOut = Common::stof(properties[i + 1]);
			break;
		case 48:
			effectPtr->pulseMode = Common::stoi(properties[i + 1]);
			break;
		case 49: {
			std::vector<std::string_view> hsv = Common::splitByDelimStringView(properties[i + 1], 'a');
			if (hsv.size() >= 5)
			{
				effectPtr->copyColorHSV.h = Common::stof(hsv[0]);
				effectPtr->copyColorHSV.s = Common::stof(hsv[1]);
				effectPtr->copyColorHSV.v = Common::stof(hsv[2]);
				effectPtr->copyColorHSV.sChecked = Common::stoi(hsv[3]);
				effectPtr->copyColorHSV.vChecked = Common::stoi(hsv[4]);
			}
		}
		break;
		case 50:
			effectPtr->copyColorID = Common::stoi(properties[i + 1]);
			break;
		case 51:
			if (ptr->isTrigger)
				effectPtr->targetGroupId = Common::stoi(properties[i + 1]);
			break;
		case 52:
			effectPtr->pulseType = Common::stoi(properties[i + 1]);
			break;
		case 56:
			if (ptr->isTrigger)
				effectPtr->activateGroup = Common::stoi(properties[i + 1]);
			break;
		case 57: {
			std::vector<std::string_view> groups = Common::splitByDelimStringView(properties[i + 1], '.');
			ptr->groups.reserve(groups.size());
			for (std::string_view groupStr : groups)
			{
				int group = Common::stoi(groupStr);
				if (group < 0 || static_cast<size_t>(group) >= GameLayer::instance->groups.size())
					continue;

				if (std::find(ptr->groups.begin(), ptr->groups.end(), group) != ptr->groups.end())
					continue;
				ptr->groups.push_back(group);

				Group& targetGroup = *GameLayer::instance->groups[group];
				targetGroup.objects.push_back(ptr.get());
				if (static_cast<size_t>(ptr->section) >= targetGroup.objectsInSections.size())
					targetGroup.objectsInSections.resize(ptr->section + 1);
				targetGroup.objectsInSections[ptr->section].push_back(ptr.get());
			}
			break;
		}
		case 58:
			effectPtr->lockPlayerX = Common::stoi(properties[i + 1]);
			break;
		case 59:
			effectPtr->lockPlayerY = Common::stoi(properties[i + 1]);
			break;
		case 62:
			effectPtr->spawnTriggered = Common::stoi(properties[i + 1]);
			break;
		case 63:
			effectPtr->spawnDelay = Common::stof(properties[i + 1]);
			break;
		case 65:
			effectPtr->mainOnly = Common::stoi(properties[i + 1]);
			break;
		case 66:
			effectPtr->detailOnly = Common::stoi(properties[i + 1]);
			break;
		case 68:
			effectPtr->degrees = Common::stoi(properties[i + 1]);
			break;
		case 69:
			effectPtr->times360 = Common::stoi(properties[i + 1]);
			break;
		case 70:
			effectPtr->lockRotation = Common::stoi(properties[i + 1]);
			break;
		case 71:
			effectPtr->secondaryTargetGroupId = Common::stoi(properties[i + 1]);
			break;
		case 72:
			effectPtr->xMod = Common::stof(properties[i + 1]);
			break;
		case 73:
			effectPtr->yMod = Common::stof(properties[i + 1]);
			break;
		case 85:
			effectPtr->easeRate = Common::stof(properties[i + 1]);
			break;
		case 87:
			effectPtr->multiActivate = Common::stof(properties[i + 1]);
			break;
		}
	}

	if (ptr->isTrigger && effectPtr->spawnTriggered && ptr->groups.size() > 0)
	{
		for (int i : ptr->groups)
		{
			GameLayer::instance->groups[i]->spawnTriggered.push_back(effectPtr.get());
		}
	}

	if (objectEntry.contains("color_type"))
	{
		if (objectEntry.contains("color_type") &&
			(objectEntry["color_type"] == "Base" || objectEntry.contains("swap_base_detail") &&
													   objectEntry["color_type"] == "Detail" &&
													   objectEntry["swap_base_detail"]))
		{
			ptr->channel = ResolveColorChannel(ptr->primaryColorChannel, 1010);
			ptr->hsvModifier = &ptr->primaryHSV;
			ptr->channelType = 1;
		}
		else if (objectEntry["color_type"] == "Detail")
		{
			ptr->channel = ResolveColorChannel(ptr->secondaryColorChannel, ptr->primaryColorChannel);
			ptr->hsvModifier = &ptr->secondaryHSV;
			ptr->channelType = 2;
		}
		else
		{
			ptr->channel = ResolveColorChannel(1010, 1010);
			ptr->channelType = 0;
		}
	}

	ptr->parent = ptr.get();

	ptr->addToChannelSection();

	ptr->setupCustomObjects(objectEntry, ptr);

	ptr->startPosition = ptr->getPosition();

	return ptr;
}

void GameObject::updateOpacity()
{
	opacityMultiplier = 1.f;

	for (int i : groups)
	{
		opacityMultiplier *= GameLayer::instance->groups[i]->groupOpacity;
	}

	setOpacityWithoutSend(opacity);

	for (std::shared_ptr<Sprite>& sprite : childSprites)
	{
		sprite->opacityMultiplier = opacityMultiplier;
		sprite->setOpacityWithoutSend(sprite->opacity);
	}
}

void GameObject::updatePosition(bool send)
{
	this->updateVerticesPosition(send);

	for (std::shared_ptr<Sprite> child : childSprites)
		child->updateVerticesPosition(send);
}

void GameObject::tryUpdateSection()
{
	int section = Common::sectionForPos(getPosition().x);
	section = section - 1 < 0 ? 0 : section - 1;

	int oldSection = this->section;
	if (oldSection == section)
		return;

	removeFromChannel();
	for (const auto& sprite : childSprites)
		sprite->removeFromChannel();

	auto& sections = GameLayer::instance->sectionObjects;
	if (oldSection >= 0 && static_cast<size_t>(oldSection) < sections.size())
		std::erase(sections[oldSection], this);
	if (static_cast<size_t>(section) >= sections.size())
		sections.resize(section + 1);
	if (std::find(sections[section].begin(), sections[section].end(), this) == sections[section].end())
		sections[section].push_back(this);
	this->section = section;

	for (int groupId : groups)
	{
		if (groupId < 0 || static_cast<size_t>(groupId) >= GameLayer::instance->groups.size())
			continue;
		std::shared_ptr<Group> group = GameLayer::instance->groups[groupId];
		if (oldSection >= 0 && static_cast<size_t>(oldSection) < group->objectsInSections.size())
			std::erase(group->objectsInSections[oldSection], this);
		if (static_cast<size_t>(section) >= group->objectsInSections.size())
			group->objectsInSections.resize(section + 1);
		auto& groupSection = group->objectsInSections[section];
		if (std::find(groupSection.begin(), groupSection.end(), this) == groupSection.end())
			groupSection.push_back(this);
	}

	addToChannelSection();
	for (const auto& sprite : childSprites)
		sprite->addToChannelSection();
}
