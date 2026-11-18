#pragma once

#include <string>
#include <memory>
#include <glm/vec2.hpp>

#include "Layer.h"

class Application
{
  private:

  float lastFrame = 0;

  std::shared_ptr<Layer> currentLayer;
  std::shared_ptr<Layer> pendingLayer;

  public:

  float deltaTime = 0, totalTime = 0;

  glm::vec2 resolution;

  static Application* instance;

  public:
	void init(glm::vec2 resolution, std::string name);

  void startUpdate();
	void update();

	void draw();

	void exit();

  void pushLayer(std::shared_ptr<Layer> layer);
  void popLayer();
};