#pragma once

#include <iostream>
#include <memory>

class Layer
{
  public:
	static std::shared_ptr<Layer> create();

	bool init();

	virtual void update();

	virtual void draw();
	virtual void GUI();

	virtual void exit();
};