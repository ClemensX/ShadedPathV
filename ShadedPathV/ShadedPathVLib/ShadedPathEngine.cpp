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


VkExtent2D ShadedPathEngine::getBackBufferExtent()
{
    return backBufferExtent;
}


void ShadedPathEngine::prepareDrawing()
{
    for (ThreadResources& tr : threadResources) {
        tr.createCommandBufferTriangle();
    }
    //for (ThreadResources& tr : threadResources) {
    //    shaders.createCommandBufferBackBufferImageDump(tr);
    //}
}

void ShadedPathEngine::drawFrame()
{
    ThemedTimer::getInstance()->add("DrawFrame");
    shaders.drawFrame_Triangle();
    shaders.executeBufferImageDump();
    frameNum++;
    currentFrameIndex = frameNum % framesInFlight;
}

void ShadedPathEngine::pollEvents()
{
}

void ShadedPathEngine::setBackBufferResolution(VkExtent2D e)
{
    backBufferExtent = e;
}

VkExtent2D ShadedPathEngine::getExtentForResolution(ShadedPathEngine::Resolution res)
{
    switch (res) {
    case Resolution::FourK:
        return VkExtent2D(3840, 2160);
    case Resolution::TwoK:
        return VkExtent2D(1920, 1080);
    case Resolution::OneK:
    case Resolution::DeviceDefault:
        return VkExtent2D(960, 540);
    case Resolution::Small:
        return VkExtent2D(480, 270);
    default:
        return VkExtent2D(960, 540);
    }
}

void ShadedPathEngine::setBackBufferResolution(ShadedPathEngine::Resolution res)
{
    setBackBufferResolution(getExtentForResolution(res));
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
