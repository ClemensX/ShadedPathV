#include "pch.h"

using namespace std;

void ShadedPathEngine::init(string appname)
{
    this->appname = appname;
    Log("engine absolute start time (hours and fraction): " << gameTime.getTimeSystemClock() << endl);
    ThemedTimer::getInstance()->create(TIMER_DRAW_FRAME, 1000);
    ThemedTimer::getInstance()->create(TIMER_PRESENT_FRAME, 1000);
    ThemedTimer::getInstance()->create(TIMER_INPUT_THREAD, 1000);
    ThemedTimer::getInstance()->create(TIMER_PART_BACKBUFFER_COPY_AND_PRESENT, 1000);
    ThemedTimer::getInstance()->create(TIMER_PART_BUFFER_COPY, 10);
    presentation.initGLFW(enabledKeyEvents, enabledMouseMoveEvents, enabledMousButtonEvents);
    vr.init();
    global.initBeforePresentation();
    presentation.init();
    global.initAfterPresentation();
    presentation.initAfterDeviceCreation();
    ThreadResources::initAll(this);
    textureStore.init(this, maxTextures);
    meshStore.init(this);
    if (soundEnabled) sound.init();
    initialized = true;
}

void ShadedPathEngine::enablePresentation(int w, int h, const char* name) {
    if (initialized) Error("Configuration after intialization not allowed");
    if (limitFrameCountEnabled) Error("Only one of presentation or frameCountLimit can be active");
    win_width = w;
    win_height = h;
    win_name = name;
    presentation.enabled = true;
    checkAspect();
};

void ShadedPathEngine::enableUI() {
    if (initialized) Error("Configuration after intialization not allowed");
    if (!presentation.enabled) Error("UI overlay needs presentation enabled");
    ui.enable();
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
    state = RENDERING;
    global.logDeviceLimits();
    if (!initialized) Error("Engine was not initialized");
    for (int i = 0; i < threadResources.size(); i++) {
        auto& tr = threadResources[i];
        tr.frameIndex = i;
        shaders.createCommandBuffers(tr);
        string name = "renderContinueQueue_" + to_string(i);
        tr.renderThreadContinueQueue.setLoggingInfo(LOG_RENDER_CONTINUATION, name);
        tr.renderThreadContinueQueue.push(0);
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
    if (threadModeSingle) {
        ThemedTimer::getInstance()->add(TIMER_DRAW_FRAME);
        auto& tr = threadResources[currentFrameIndex];
        drawFrame(tr);
        shaders.queueSubmit(tr);
        presentation.presentBackBufferImage(tr);
        ThemedTimer::getInstance()->add(TIMER_PRESENT_FRAME);
        advanceFrameCountersAfterPresentation();
    }
}

void ShadedPathEngine::drawFrame(ThreadResources& tr)
{
    // set new frameNum
    long oldNum = tr.frameNum;
    tr.frameNum = getNextFrameNumber();
    assert(oldNum < tr.frameNum);
    // wait for fence signal
    LogCondF(LOG_QUEUE, "wait drawFrame() present fence image index " << tr.frameIndex << endl);
    LogCondF(LOG_FENCE, "render thread wait present fence " << hex << ThreadInfo::thread_osid() << endl);
    vkWaitForFences(global.device, 1, &tr.presentFence, VK_TRUE, UINT64_MAX);
    vkResetFences(global.device, 1, &tr.presentFence);
    LogCondF(LOG_QUEUE, "fence drawFrame() present fence signalled image index " << tr.frameIndex << endl);

    if (app == nullptr) {
        Error("No app configured. Call ShadedPathEngine::registerApp(ShadedPathApplication* app)");
    }
    app->drawFrame(tr);
    shaders.executeBufferImageDump(tr);
}

void ShadedPathEngine::pollEvents()
{
    if (!initialized) Error("Engine was not initialized");
    presentation.pollEvents();
    ui.update();

    // measure how often we run the input cycle:
    if (!threadModeSingle) {
        ThemedTimer::getInstance()->add(TIMER_INPUT_THREAD);
        limiter.waitForLimit();
        //Log("Input Thread\n");
    }
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
    checkAspect();
}

void ShadedPathEngine::checkAspect()
{
    // nothing to check if presentation in not enabled
    if (!presentation.enabled) return;
    float winaspect = (float)win_width / (float)win_height;
    bool aspectsMatch = glm::epsilonEqual(winaspect, getAspect(), 0.1f);
    if (!aspectsMatch) {
        Log("WARNING: aspect of window does not match backbuffer - you risk distorted views");
    }
}

bool ShadedPathEngine::shouldClose()
{
    if (presentation.enabled && presentation.shouldClose()) {
        if (threadModeSingle) {
            return true;
        }
        else {
            if (!isShutdown()) {
                // initialte shutdown
                shutdown();
                return false;
            }
            else {
                // wait until threads have died
                return threadsAreFinished();
            }
        }
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
    LogCondF(LOG_QUEUE, "run DrawFrame start " << tr->frameIndex << endl);
    //this_thread::sleep_for(chrono::milliseconds(1000 * (10 - tr->frameIndex)));
    while (engine_instance->isShutdown() == false) {
        // wait until queue submit thread issued all present commands
//        tr->renderThreadContinue->wait(false);
        optional<unsigned long> o = tr->renderThreadContinueQueue.pop();
        if (!o) {
            break;
        }
        // draw next frame
        engine_instance->drawFrame(*tr);
        engine_instance->queue.push(tr);
        LogCondF(LOG_QUEUE, "pushed frame: " << tr->frameNum << endl);
    }
    LogCondF(LOG_QUEUE, "run DrawFrame end " << tr->frameIndex << endl);
    tr->threadFinished = true;
}

void ShadedPathEngine::runQueueSubmit(ShadedPathEngine* engine_instance)
{
    LogF("run QueueSubmit start " << endl);
    //pipeline_instance->setRunning(true);
    while (engine_instance->isShutdown() == false) {
        auto v = engine_instance->queue.pop();
        if (v == nullptr) {
            LogF("engine shutdown" << endl);
            break;
        }
        LogCondF(LOG_QUEUE, "engine received frame: " << v->frameNum << endl);
        engine_instance->shaders.queueSubmit(*v);
        // if we are pop()ed by drawing thread we can be sure to own the thread until presentFence is signalled,
        // we still have to wat for inFlightFence to make sure rendering has ended
        ThemedTimer::getInstance()->start(TIMER_PART_BACKBUFFER_COPY_AND_PRESENT);
        engine_instance->presentation.presentBackBufferImage(*v);
        if (v->frameNum != engine_instance->frameNum) {
            LogF("Frames async: drawing:" << v->frameNum << " present: " << engine_instance->frameNum << endl);
            //Error("Frames out of sync");
        }
        engine_instance->advanceFrameCountersAfterPresentation();
        //this_thread::sleep_for(chrono::milliseconds(5000));
        //Log("rel time: " << engine_instance->gameTime.getRealTimeDelta() << endl);
        ThemedTimer::getInstance()->stop(TIMER_PART_BACKBUFFER_COPY_AND_PRESENT);
        ThemedTimer::getInstance()->add(TIMER_PRESENT_FRAME);
        // tell render thread to continue:
        //v->renderThreadContinue->test_and_set();
        //v->renderThreadContinue->notify_one();
        v->renderThreadContinueQueue.push(0);
    }
    //engine_instance->setRunning(false);
    LogF("run QueueSubmit end " << endl);
    queueThreadFinished = true;
}

bool ShadedPathEngine::queueThreadFinished = false;

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

bool ShadedPathEngine::isGlobalUpdateThread(ThreadResources& tr)
{
    // checking for frame index == 0 should be ok for single and multi thread mode.
    return tr.frameIndex == 0;
}

long ShadedPathEngine::getNextFrameNumber()
{
    long n = nextFreeFrameNum++;
    return n;
}

void ShadedPathEngine::advanceFrameCountersAfterPresentation()
{
    frameNum++;
    currentFrameIndex = frameNum % framesInFlight;
    gameTime.advanceTime();
    fpsCounter.tick(gameTime.getRealTimeDelta(), true);
}

void ShadedPathEngine::shutdown()
{
    shutdown_mode = true;
    /*queue.shutdown();*/
    for (int i = 0; i < framesInFlight; i++) {
        auto& tr = threadResources[i];
        tr.renderThreadContinueQueue.shutdown();
    }
}

void ShadedPathEngine::waitUntilShutdown()
{
    if (!threadModeSingle) {
        queue.shutdown();
        threads.join_all();
    }
}

bool ShadedPathEngine::threadsAreFinished()
{
    if (!queueThreadFinished) return false;
    for (int i = 0; i < framesInFlight; i++) {
        auto* tr = &threadResources[i];
        if (!tr->threadFinished) return false;
    }
    return true;
}

ShadedPathEngine::~ShadedPathEngine()
{
    shutdown();
    Log("Engine destructor\n");
    if (global.device) vkDeviceWaitIdle(global.device);
    //ThemedTimer::getInstance()->logInfo(TIMER_DRAW_FRAME);
    //ThemedTimer::getInstance()->logFPS(TIMER_DRAW_FRAME);
    ThemedTimer::getInstance()->logInfo(TIMER_PRESENT_FRAME);
    ThemedTimer::getInstance()->logFPS(TIMER_PRESENT_FRAME);
    ThemedTimer::getInstance()->logInfo(TIMER_INPUT_THREAD);
    ThemedTimer::getInstance()->logFPS(TIMER_INPUT_THREAD);
    ThemedTimer::getInstance()->logInfo(TIMER_PART_BACKBUFFER_COPY_AND_PRESENT);
    ThemedTimer::getInstance()->logInfo(TIMER_PART_BUFFER_COPY);
}

