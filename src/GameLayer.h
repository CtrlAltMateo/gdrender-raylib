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

#include <unordered_map>
#include <unordered_set>

#include <raylib.h>

extern Camera2D camera;

class GameLayer : public Layer
{
  private:
	float lastObjXPos = 570.f;

	int bgID = 1;

  public:
	std::shared_ptr<Batcher> gameSheet01_t3;
	std::shared_ptr<Batcher> gameSheet01_t3_blending;
	std::shared_ptr<Batcher> gameSheet01_t2;
	std::shared_ptr<Batcher> gameSheet01_t2_blending;
	std::shared_ptr<Batcher> gameSheet01_t1;
	std::shared_ptr<Batcher> gameSheet01_t1_blending;
	std::shared_ptr<Batcher> gameSheet01_b4;
	std::shared_ptr<Batcher> gameSheet01_b4_blending;
	std::shared_ptr<Batcher> gameSheet01_b3;
	std::shared_ptr<Batcher> gameSheet01_b3_blending;
	std::shared_ptr<Batcher> gameSheet01_b2;
	std::shared_ptr<Batcher> gameSheet01_b2_blending;
	std::shared_ptr<Batcher> gameSheet01_b1;
	std::shared_ptr<Batcher> gameSheet01_b1_blending;
	std::shared_ptr<Batcher> gameSheet02;

	std::array<std::shared_ptr<Batcher>, 15> batchers;
	Scrolling scrolling;

	std::vector<std::shared_ptr<GameObject>> objects;
	std::vector<std::unordered_map<int, GameObject*>> sectionObjects;
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

	std::array<std::shared_ptr<ColorChannel>, 1013> colorChannels;
	std::array<std::shared_ptr<Group>, 1013> groups;

	int prevSection = 0;
	int nextSection = 0;
	int maxSectionSize = 0;

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