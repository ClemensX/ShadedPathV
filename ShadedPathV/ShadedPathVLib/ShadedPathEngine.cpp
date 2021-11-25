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
    initialized = true;
}

void ShadedPathEngine::enablePresentation(int w, int h, const char* name) {
    if (initialized) Error("Configuration after intialization not allowed");
    if (limitFrameCountEnabled) Error("Only one of presentation or frameCountLimit can be active");
    win_width = w;
    win_height = h;
    win_name = name;
    presentation.enabled = true;
};

void ShadedPathEngine::setFramesInFlight(int n) {
    if (initialized) Error("Configuration after intialization not allowed");
    framesInFlight = n;
    threadResources.resize(framesInFlight);
}

void ShadedPathEngine::setFrameCountLimit(long max) {
    if (initialized) Error("Configuration after intialization not allowed");
    if (presentation.enabled) Error("Only one of presentation or frameCountLimit can be active");
    limitFrameCount = max;
    limitFrameCountEnabled = true;
}


VkExtent2D ShadedPathEngine::getBackBufferExtent()
{
    return backBufferExtent;
}


void ShadedPathEngine::prepareDrawing()
{
    if (!initialized) Error("Engine was not initialized");
    for (ThreadResources& tr : threadResources) {
        tr.createCommandBufferTriangle();
    }
    presentation.initBackBufferPresentation();
    //for (ThreadResources& tr : threadResources) {
    //    shaders.createCommandBufferBackBufferImageDump(tr);
    //}
}

void ShadedPathEngine::drawFrame()
{
    if (!initialized) Error("Engine was not initialized");
    ThemedTimer::getInstance()->add("DrawFrame");
    shaders.drawFrame_Triangle();
    shaders.executeBufferImageDump();
    presentation.presentBackBufferImage();
    frameNum++;
    currentFrameIndex = frameNum % framesInFlight;
}

void ShadedPathEngine::pollEvents()
{
    if (!initialized) Error("Engine was not initialized");
    presentation.pollEvents();
}

void ShadedPathEngine::setBackBufferResolution(VkExtent2D e)
{
    if (initialized) Error("Configuration after intialization not allowed");
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
    if (initialized) Error("Configuration after intialization not allowed");
    setBackBufferResolution(getExtentForResolution(res));
}

bool ShadedPathEngine::shouldClose()
{
    if (presentation.enabled) {
        return presentation.shouldClose();
    }

    // max frames reached?
    if (limitFrameCountEnabled && frameNum >= limitFrameCount) {
        return true;
    }
    return false;
}

void ShadedPathEngine::shutdown()
{
    // TODO needed?
}
