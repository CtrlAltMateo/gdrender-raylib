#include <raylib.h>

#include "Application.h"
#include "PlatformUtils.h"
#include "SelectLevelLayer.h"

Camera2D camera = {};

int main()
{
    if (!PlatformUtils::updateResourcesPath())
        return 1;

    Application app;
    app.init(glm::vec2(1920, 1080), "gdrender");

    auto layer = SelectLevelLayer::create();

    app.pushLayer(layer);

    app.startUpdate();

    return 0;
}