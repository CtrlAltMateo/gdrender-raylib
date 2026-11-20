#pragma once

#include <memory>
#include <glm/vec2.hpp>

#include "Layer.h"

class Application
{
  private:

  std::shared_ptr<Layer> currentLayer;
  std::shared_ptr<Layer> pendingLayer;

  public:

  float deltaTime = 0, totalTime = 0;

  glm::vec2 resolution;
  bool mirrorX = false;

  static Application* instance;

  public:
	void update(float frameDelta = -1.0f);

	void draw(void (*overlay)() = nullptr);

	void exit();

  void pushLayer(std::shared_ptr<Layer> layer);
  void popLayer();
};