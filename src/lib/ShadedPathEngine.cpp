#include "mainheader.h"

using namespace std;

void ShadedPathEngine::initGlobal() {
    Log("initGlobal\n");
    mainThreadInfo.name = "Main Thread";
    mainThreadInfo.category = ThreadCategory::MainThread;
    mainThreadInfo.id = this_thread::get_id();
    log_current_thread();
    globalRendering.initBeforePresentation();
    globalRendering.initAfterPresentation();
}

ShadedPathEngine::~ShadedPathEngine()
{
    Log("Engine destructor\n");
    for (auto& img : images) {
        globalRendering.destroyImage(&img);
    }
}

VkExtent2D ShadedPathEngine::getBackBufferExtent()
{
    return backBufferExtent;
}

void ShadedPathEngine::setBackBufferResolution(VkExtent2D e)
{
    if (initialized) Error("Configuration after intialization not allowed");
    backBufferExtent = e;
    backBufferAspect = (float)e.width / (float)e.height;
}

VkExtent2D ShadedPathEngine::getExtentForResolution(ShadedPathEngine::Resolution res)
{
    switch (res) {
    case Resolution::FourK:
        return { 3840, 2160 };
    case Resolution::TwoK:
        return { 1920, 1080 };
    case Resolution::OneK:
    case Resolution::DeviceDefault:
        return { 960, 540 };
    case Resolution::Small:
        return { 480, 270 };
    case Resolution::Invalid:
        return { 0, 0 };
    default:
        return { 960, 540 };
    }
}

void ShadedPathEngine::setBackBufferResolution(ShadedPathEngine::Resolution res)
{
    //if (shouldCloseApp) return;
    if (initialized) Error("Configuration after intialization not allowed");
    setBackBufferResolution(getExtentForResolution(res));
    //checkAspect();
}

GPUImage* ShadedPathEngine::createImage(const char* debugName)
{
    return globalRendering.createImage(images, debugName);
}

void ShadedPathEngine::log_current_thread()
{
    // check for main thread (used in single thread mode)
    if (mainThreadInfo.id == this_thread::get_id()) {
        Log(mainThreadInfo << std::endl);
        return;
    }
    // check for worker threads
    auto& t = getThreadGroupMain().current_thread();
    Log(t << std::endl);
}

void ShadedPathEngine::eventLoop()
{
    // some shaders may need additional preparation
    //engine->prepareDrawing();


    // rendering
    while (!shouldClose()) {
        preFrame();
        drawFrame();
        postFrame();
    }
    waitUntilShutdown();

}

bool ShadedPathEngine::shouldClose()
{
    return app->shouldClose();
}

void ShadedPathEngine::preFrame()
{
    // alternate frame infos:
    long frameNum = getNextFrameNumber();
    int currentFrameInfoIndex = frameNum & 0x01;
    currentFrameInfo = &frameInfos[currentFrameInfoIndex];
    // init frame info:
    currentFrameInfo->frameNum = frameNum;

    // call app
    app->prepareFrame(currentFrameInfo);
}

void ShadedPathEngine::drawFrame()
{
}

void ShadedPathEngine::postFrame()
{
}

void ShadedPathEngine::waitUntilShutdown()
{
}

long ShadedPathEngine::getNextFrameNumber()
{
    long n = ++nextFreeFrameNum;
    return n;
}

