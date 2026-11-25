#pragma once

#include "Batcher.h"
#include "ColorAction.h"
#include "ColorChannel.h"
#include "CopyColorAction.h"
#include "FollowAction.h"
#include "GameObject.h"
#include "Group.h"
#include "Layer.h"
#include "MoveAction.h"
#include "OpacityAction.h"
#include "PulseAction.h"
#include "RotateAction.h"
#include "Scrolling.h"
#include "SpawnAction.h"

#include <unordered_set>

#include <raylib.h>

extern Camera2D camera;

class GameLayer : public Layer
{
  private:
	float lastObjXPos = 570.f;

	int bgID = 1;

  public:
	std::array<std::array<std::shared_ptr<Batcher>, 2>, 7> gameSheet01;
	std::shared_ptr<Batcher> gameSheet02;
	std::array<std::array<std::shared_ptr<Batcher>, 2>, 7> gameSheet03;
	std::array<std::array<std::shared_ptr<Batcher>, 2>, 7> gameSheet04;

	std::vector<std::shared_ptr<Batcher>> batchers;
	Scrolling scrolling;

	std::vector<std::shared_ptr<GameObject>> objects;
	std::vector<std::vector<GameObject*>> sectionObjects;
	std::vector<std::shared_ptr<ColorAction>> colorActionsActive;
	std::vector<std::shared_ptr<CopyColorAction>> copyColorActionsActive;
	std::vector<std::shared_ptr<PulseAction>> pulseActionsActive;
	std::vector<std::shared_ptr<OpacityAction>> opacityActionsActive;
	std::vector<std::shared_ptr<ActionInterval>> moveActionsActive;
	std::vector<std::shared_ptr<SpawnAction>> spawnActionsActive;
	std::vector<std::shared_ptr<SpawnAction>> spawnActionsPending;
	std::vector<std::shared_ptr<ActionInterval>> rotateActionsActive;
	std::vector<std::shared_ptr<FollowAction>> followActionsActive;
	std::unordered_set<int> dirtyChannels;
	std::unordered_set<int> dirtyGroups;

	std::unordered_set<int> pulsedGroups;

	std::array<std::shared_ptr<ColorChannel>, 2048> colorChannels;
	std::array<std::shared_ptr<Group>, 2048> groups;

	int prevSection = 0;
	int nextSection = 0;
	glm::vec2 deltaMove = {0, 0};

	static GameLayer* instance;

  public:
	static std::shared_ptr<GameLayer> createWithLevelString(std::string_view lvlstr);

	bool initWithLevelString(std::string_view lvlstr);
	bool initializeMembers();

	virtual void update() override;
	virtual void drawBackground() override;
	virtual void draw() override;
	virtual void exit() override;

	void drawForSprite(Sprite* sprite, int index);
	void drawForObject(GameObject* object, int index);

	void loadLevelFromLevelString(std::string_view levelString);
	void setupLevel(std::string_view levelStr);
	void setupObjects(std::string_view levelStr);

	void layerObject(Sprite* sprite);

	void fillColorChannel(std::span<std::string_view> colorString, int id);

	void updateVisibility();
	void updateTriggers();

	void processColorActions();
	void updateLevelObjects();

	void updateCamera();
};