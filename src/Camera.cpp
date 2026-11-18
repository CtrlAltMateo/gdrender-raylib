#include "Camera.h"

#include <algorithm>

void GameCamera::init()
{
	viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	projectionMatrix = glm::ortho(-2.f, 2.f, -2.f, 2.f, -1.f, 1000.f);
}

void GameCamera::updateTransform()
{
	glm::vec3 direction;
	direction.x = cos(glm::radians(cameraEulerAngles.x)) * cos(glm::radians(cameraEulerAngles.y));
	direction.y = sin(glm::radians(cameraEulerAngles.y));
	direction.z = sin(glm::radians(cameraEulerAngles.x)) * cos(glm::radians(cameraEulerAngles.y));

	cameraFront = glm::normalize(direction);

	viewMatrix = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));
	viewMatrix = scaleMatrix * viewMatrix;
}

void GameCamera::update()
{
	if (renderTexture && renderTexture->resolution != resolution)
		renderTexture->resize(resolution);
}

void GameCamera::resize(glm::vec2 resolution)
{
	this->resolution = resolution;

	glm::vec2 size = {1.f, resolution.y / resolution.x};
	size *= this->size;

	projectionMatrix = glm::ortho(-size.x, size.x, -size.y, size.y, 0.1f, 1000.f);
}

void GameCamera::startDraw()
{
	if(renderTexture)
		renderTexture->bind();

	Camera2D camera2D = {};
	camera2D.offset = {resolution.x / 2.0f, resolution.y / 2.0f};
	camera2D.target = {cameraPos.x, -cameraPos.y};
	camera2D.zoom = resolution.y / (2.0f * std::max(size, 1.0f));
	BeginMode2D(camera2D);
}

void GameCamera::endDraw()
{
	EndMode2D();
	if (renderTexture)
		renderTexture->unbind();
}

void GameCamera::clear()
{
	if (renderTexture)
		renderTexture->clear();
	else
		ClearBackground({77, 77, 77, 255});
}
