#pragma once

#include "RenderTexture.h"

#include <glm/gtc/matrix_transform.hpp>

class GameCamera
{
  public:
	float size = 400.f;

	glm::vec2 resolution = {800, 800};

	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::vec3 cameraEulerAngles = glm::vec3(-90.f, 0.0f, 0.0f);

	GameRenderTexture* renderTexture = nullptr;

	bool draw = true;

  public:
	void init();

	void update();
	void updateTransform();

	void resize(glm::vec2 resolution);

	void clear();

	void startDraw();
	void endDraw();
};