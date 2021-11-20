#include "pch.h"


void ShadedPathEngine::init()
{
    Log("engine absolute start time (hours and fraction): " << gameTime.getTimeAbs() << endl);
    ThemedTimer::getInstance()->create("DrawFrame", 1000);
    presentation.initGLFW();
    global.initBeforePresentation();
    presentation.init();
    global.initAfterPresentation();
    presentation.initAfterDeviceCreation();
    ThreadResources::initAll(this);
}


VkExtent2D ShadedPathEngine::getCurrentExtent()
{
    return currentExtent;
}


void ShadedPathEngine::prepareDrawing()
{
    for (ThreadResources& tr : threadResources) {
        tr.createCommandBufferTriangle();
    }
}

void ShadedPathEngine::drawFrame()
{
    ThemedTimer::getInstance()->add("DrawFrame");
    shaders.drawFrame_Triangle();
    frameNum++;
}

void ShadedPathEngine::pollEvents()
{
}

bool ShadedPathEngine::shouldClose()
{
    if (presentation.enabled) {
        return presentation.shouldClose();
    }

    // max frames reached?
    if (frameNum >= limitFrameCount) {
        return true;
    }
    return false;
}

void ShadedPathEngine::shutdown()
{
    // TODO needed?
}
