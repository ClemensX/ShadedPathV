#include "mainheader.h"

using namespace std;

void ShadedPathEngine::initGlobal() {
    Log("initGlobal\n");
    Log("engine absolute start time (hours and fraction): " << gameTime.getTimeSystemClock() << endl);
    ThemedTimer::getInstance()->create(TIMER_DRAW_FRAME, 1000);
    ThemedTimer::getInstance()->create(TIMER_PRESENT_FRAME, 1000);
    ThemedTimer::getInstance()->create(TIMER_INPUT_THREAD, 1000);
    ThemedTimer::getInstance()->create(TIMER_PART_BACKBUFFER_COPY_AND_PRESENT, 1000);
    ThemedTimer::getInstance()->create(TIMER_PART_GLOBAL_UPDATE, 1000);
    ThemedTimer::getInstance()->create(TIMER_PART_OPENXR, 1000);
    ThemedTimer::getInstance()->create(TIMER_PART_BUFFER_COPY, 10);
    mainThreadInfo.name = "Main Thread";
    mainThreadInfo.category = ThreadCategory::MainThread;
    mainThreadInfo.id = this_thread::get_id();
    log_current_thread();
    numCores = std::thread::hardware_concurrency();
    if (numCores == 0) {
        Log("Unable to detect the number of CPU cores." << std::endl);
    }
    else {
        Log("Number of CPU cores: " << numCores << std::endl);
    }
    if (!singleThreadMode && overrideUsedCores >= 0) {
        Log("Overriding number of used CPU cores to: " << overrideUsedCores << std::endl);
        numCores = overrideUsedCores;
    }
    if (!singleThreadMode) {
        if (numCores < 4) Error("You cannot run in multi core mode with less than 4 cores assigned");
        Log("Multi Core mode creating " << numCores - 2 << " worker threads\n");
        threadsWorker = new ThreadGroup(numCores - 2);
//        workerFutures = new future<void>[numCores - 2];
        workerFutures.resize(numCores - 2); // Initialize the vector with the appropriate size

    }

    globalRendering.initBeforePresentation();
    globalRendering.initAfterPresentation();
}

ShadedPathEngine::~ShadedPathEngine()
{
    Log("Engine destructor\n");
    for (auto& img : images) {
        globalRendering.destroyImage(&img);
    }
    if (globalRendering.device) vkDeviceWaitIdle(globalRendering.device);
    if (threadsWorker) delete threadsWorker;
    //if (workerFutures) delete workerFutures;
    ThemedTimer::getInstance()->logInfo(TIMER_DRAW_FRAME);
    //ThemedTimer::getInstance()->logFPS(TIMER_DRAW_FRAME);
    ThemedTimer::getInstance()->logInfo(TIMER_PRESENT_FRAME);
    ThemedTimer::getInstance()->logFPS(TIMER_PRESENT_FRAME);
    ThemedTimer::getInstance()->logInfo(TIMER_INPUT_THREAD);
    ThemedTimer::getInstance()->logFPS(TIMER_INPUT_THREAD);
    ThemedTimer::getInstance()->logInfo(TIMER_PART_BACKBUFFER_COPY_AND_PRESENT);
    ThemedTimer::getInstance()->logInfo(TIMER_PART_BUFFER_COPY);
    ThemedTimer::getInstance()->logInfo(TIMER_PART_GLOBAL_UPDATE);
    ThemedTimer::getInstance()->logInfo(TIMER_PART_OPENXR);
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

bool ShadedPathEngine::isMainThread()
{
    return mainThreadInfo.id == this_thread::get_id();
}

void ShadedPathEngine::log_current_thread()
{
    // check for main thread (used in single thread mode)
    if (isMainThread()) {
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

void ShadedPathEngine::initFrame(FrameInfo* fi, long frameNum)
{
    fi->frameNum = frameNum;
    ThemedTimer::getInstance()->start(TIMER_DRAW_FRAME);
}

void ShadedPathEngine::preFrame()
{
    // input
    app->mainThreadHook();
    presentation.pollEvents();
    if (!threadModeSingle) {
        ThemedTimer::getInstance()->add(TIMER_INPUT_THREAD);
        //limiter.waitForLimit();
    }
    // alternate frame infos:
    long frameNum = getNextFrameNumber();
    int currentFrameInfoIndex = frameNum & 0x01;
    currentFrameInfo = &frameInfos[currentFrameInfoIndex];
    initFrame(currentFrameInfo, frameNum);

    // call app
    app->prepareFrame(currentFrameInfo);
}

GPUImage* ShadedPathEngine::drawFrame()
{
    // call app
    if (singleThreadMode) {
        for (int i = 0; i < appDrawCalls; i++) {
            app->drawFrame(currentFrameInfo, i);
        }
    } else {
        for (int i = 0; i < appDrawCalls; i++) {
            workerFutures[i] = threadsWorker->asyncSubmit([this, i] {
                app->drawFrame(currentFrameInfo, i);
            });
        }
        // we must wait for all draw calls to finish
        for (int i = 0; i < appDrawCalls; i++) {
            workerFutures[i].wait();
        }
    }
    // initiate shader runs via job system
    if (currentFrameInfo->renderedImage->rendered == false) {
        Error("Image not rendered");
    }
    lastImage = currentFrameInfo->renderedImage; // TODO check
    return currentFrameInfo->renderedImage;
}

void ShadedPathEngine::postFrame()
{
    if (singleThreadMode) {
        ThemedTimer::getInstance()->stop(TIMER_DRAW_FRAME);
        singleThreadPostFrame();
    } else {
        //Error("Multi thread mode not implemented");
        // do the same in multi thread mode (for now)
        ThemedTimer::getInstance()->stop(TIMER_DRAW_FRAME);
        singleThreadPostFrame();
    }
}

void ShadedPathEngine::singleThreadPostFrame()
{
    // consume image
    // advance sound
    // etc.
    if (imageConsumer == nullptr) {
        Log("WARNING: No image consumer set, defaulting to discarding image\n");
        imageConsumer = &imageConsumerNullify;
    }
    imageConsumer->consume(lastImage);
}

void ShadedPathEngine::waitUntilShutdown()
{
}

long ShadedPathEngine::getNextFrameNumber()
{
    long n = ++nextFreeFrameNum;
    return n;
}

void ShadedPathEngine::enablePresentation(WindowInfo* winfo, int w, int h, const char* name)
{
    presentation.createWindow(winfo, w, h, name);
}
