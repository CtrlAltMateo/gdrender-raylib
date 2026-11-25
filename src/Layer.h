#pragma once

class Layer
{
  public:
	virtual ~Layer() = default;
	virtual void update() {}
	virtual void drawBackground() {}
	virtual void draw() {}
	virtual void exit() {}
};