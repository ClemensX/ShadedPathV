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
    for (int i = 0; i < threadResources.size(); i++) {
        auto& tr = threadResources[i];
        tr.frameIndex = i;
        tr.createCommandBufferTriangle();
    }
    presentation.initBackBufferPresentation();

    if (!threadModeSingle) {
        startQueueSubmitThread();
        startRenderThreads();
    }
    //for (ThreadResources& tr : threadResources) {
    //    shaders.createCommandBufferBackBufferImageDump(tr);
    //}
}

void ShadedPathEngine::drawFrame()
{
    if (!initialized) Error("Engine was not initialized");
    ThemedTimer::getInstance()->add("DrawFrame");
    if (threadModeSingle) {
        auto& tr = threadResources[currentFrameIndex];
        drawFrame(tr);
        shaders.queueSubmit(tr);
        presentation.presentBackBufferImage(tr);
    }
    frameNum++;
    currentFrameIndex = frameNum % framesInFlight;
}

void ShadedPathEngine::drawFrame(ThreadResources& tr)
{
    shaders.drawFrame_Triangle(tr);
    shaders.executeBufferImageDump(tr);
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

// multi threading

void ShadedPathEngine::runDrawFrame(ShadedPathEngine* engine_instance, ThreadResources* tr)
{
    LogF("run DrawFrame start " << tr->frameIndex << endl);
    //this_thread::sleep_for(chrono::milliseconds(1000 * (10 - tr->frameIndex)));
    while (engine_instance->isShutdown() == false) {
        engine_instance->drawFrame(*tr); // TODO not really a copy, right?
        engine_instance->queue.push(tr);
        LogF("pushed frame: " << tr->frameNum << endl);
    }
    LogF("run DrawFrame end " << tr->frameIndex << endl);
}

void ShadedPathEngine::runQueueSubmit(ShadedPathEngine* engine_instance)
{
    LogF("run QueueSubmit start " << endl);
    //pipeline_instance->setRunning(true);
    while (engine_instance->isShutdown() == false) {
        auto v = engine_instance->queue.pop();
        if (v == nullptr) {
            LogF("pipeline shutdown" << endl);
            break;
        }
        LogF("pipeline received frame: " << v->frameNum << endl);
        engine_instance->shaders.queueSubmit(*v);
        engine_instance->presentation.presentBackBufferImage(*v);
    }
    //engine_instance->setRunning(false);
    LogF("run QueueSubmit end " << endl);
}

void ShadedPathEngine::startRenderThreads()
{
    if (!initialized) {
        Error("cannot start render threads: pipeline not initialized\n");
        return;
    }
    //if (consumer == nullptr) {
    //    Error("cannot start render threads: no frame consumer specified\n");
    //    return;
    //}
    //pipelineStartTime = chrono::high_resolution_clock::now();
    for (int i = 0; i < framesInFlight; i++) {
        auto* tr = &threadResources[i];
        void* native_handle = threads.add_t(runDrawFrame, this, tr);
        wstring mod_name = wstring(L"render_thread").append(L"_").append(to_wstring(i));
        SetThreadDescription((HANDLE)native_handle, mod_name.c_str());
    }
}

void ShadedPathEngine::startQueueSubmitThread()
{
    if (!initialized) {
        Error("cannot start update thread: pipeline not initialized\n");
        return;
    }
    //if (updateCallback == nullptr) {
    //    Error(L"cannot start update thread: no update consumer specified\n");
    //    return;
    //}
    // one queue submit thread for all threads:
    void* native_handle = threads.add_t(runQueueSubmit, this);
    wstring mod_name = wstring(L"queue_submit");//.append(L"_").append(to_wstring(i));
    SetThreadDescription((HANDLE)native_handle, mod_name.c_str());
}

