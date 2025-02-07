#include "mainheader.h"

using namespace std;

void ShadedPathEngine::initGlobal(string appname) {
    if (!appname.empty()) {
        this->appname = appname;
    }
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
        numWorkerThreads = numCores - 2;
        threadsWorker = new ThreadGroup(numWorkerThreads);
        workerFutures.resize(numWorkerThreads); // Initialize the vector with the appropriate size
    } else {
        numWorkerThreads = 1;
    }
    vr.init();
    globalRendering.init();
    // init frame infos index:
    for (int i = 0; i < 2; i++) {
        frameInfos[i].engine = this;
        frameInfos[i].frameIndex = i;
    }
    //FrameResources::initAll(this);
    textureStore.init(this, maxTextures);
    //meshStore.init(this);
    //if (soundEnabled) sound.init();
    initialized = true;
}

ShadedPathEngine::~ShadedPathEngine()
{
    Log("Engine destructor\n");
    while (!windowInfos.empty()) {
        WindowInfo* lastWindowInfo = windowInfos.back();
        windowInfos.pop_back();
        presentation.destroyWindowResources(lastWindowInfo);
    }
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
    //if (initialized) Error("Configuration after intialization not allowed");
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
    //if (initialized) Error("Configuration after intialization not allowed");
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
    prepareDrawing();

    // rendering
    while (!shouldClose()) {
        // input
        app->mainThreadHook();
        presentation.pollEvents();
        if (!singleThreadMode) {
            ThemedTimer::getInstance()->add(TIMER_INPUT_THREAD);
            //limiter.waitForLimit();
        } else {
            // frame generation will only be called from main thread in single thread mode
            preFrame();
            drawFrame();
            postFrame();
        }
    }
    waitUntilShutdown();

}

void ShadedPathEngine::prepareDrawing()
{
    globalRendering.logDeviceLimits();
    if (!initialized) Error("Engine was not initialized");
    //for (int i = 0; i < threadResources.size(); i++) {
    //    auto& tr = threadResources[i];
    //    tr.frameIndex = i;
    //    shaders.createCommandBuffers(tr);
    //    string name = "renderContinueQueue_" + to_string(i);
    //    tr.renderThreadContinueQueue.setLoggingInfo(LOG_RENDER_CONTINUATION, name);
    //    tr.renderThreadContinueQueue.push(0);
    //}
    //presentation.initBackBufferPresentation();
    // check frame resources:
    for (auto& fi : frameInfos) {
        if (fi.drawResults.size() < appDrawCalls) {
            Error("Frames have not been properly initialized. Did you forget initActiveShaders()?");
        }
        shaders.createCommandBuffers(fi);
    }
    if (!singleThreadMode) {
        qsr.renderThreadContinueQueue.setLoggingInfo(LOG_RENDER_CONTINUATION, "renderContinueQueue");
        qsr.renderThreadContinueQueue.push(0);
        startQueueSubmitThread();
        startRenderThread();
        //startUpdateThread();
    }
    //for (ThreadResources& tr : threadResources) {
    //    shaders.createCommandBufferBackBufferImageDump(tr);
    //}
}

void ShadedPathEngine::startQueueSubmitThread()
{
    if (!initialized) {
        Error("cannot start update thread: pipeline not initialized\n");
        return;
    }
    // one queue submit thread for all threads:
    threadsMain.addThread(ThreadCategory::DrawQueueSubmit, "queue_submit", runQueueSubmit, this);
}

void ShadedPathEngine::startRenderThread()
{
    if (!initialized) {
        Error("cannot start render threads: pipeline not initialized\n");
        return;
    }
    std::string name = "main_render_thread";
    threadsMain.addThread(ThreadCategory::Draw, name, runDrawFrame, this);
    //if (framesInFlight > 1 && isVR()) {
    //    Error("cannot start render threads: VR mode not supported with multiple render threads. Use engine.setFramesInFlight(1)\n");
    //}
    //if (consumer == nullptr) {
    //    Error("cannot start render threads: no frame consumer specified\n");
    //    return;
    //}
    //pipelineStartTime = chrono::high_resolution_clock::now();
    //for (int i = 0; i < framesInFlight; i++) {
    //    auto* tr = &threadResources[i];
    //    std::string name = "render_thread_" + std::to_string(i);
    //    threads.addThread(ThreadCategory::Draw, name, runDrawFrame, this, tr);
    //}
}

bool ShadedPathEngine::shouldClose()
{
    return app->shouldClose();
}

void ShadedPathEngine::initFrame(FrameResources* fi, long frameNum)
{
    fi->frameNum = frameNum;
    ThemedTimer::getInstance()->start(TIMER_DRAW_FRAME);
}

void ShadedPathEngine::preFrame()
{
    // alternate frame infos:
    long frameNum = getNextFrameNumber();
    int currentFrameInfoIndex = frameNum & 0x01;
    currentFrameInfo = &frameInfos[currentFrameInfoIndex];
    gameTime.advanceTime();
    fpsCounter.tick(gameTime.getRealTimeDelta(), true);
    initFrame(currentFrameInfo, frameNum);
    globalRendering.preFrame(currentFrameInfo);

    // call app
    app->prepareFrame(currentFrameInfo);

}

void ShadedPathEngine::drawFrame()
{
    // call app
    currentFrameInfo->drawFrameDone = false;
    if (singleThreadMode) {
        for (int i = 0; i < appDrawCalls; i++) {
            app->drawFrame(currentFrameInfo, i, &currentFrameInfo->drawResults[i]);
        }
    } else {
        for (int i = 0; i < appDrawCalls; i++) {
            if (appDrawCalls > threadsWorker->size()) {
                Error("App wants more parallel calls than there are worker threads!");
            }
            workerFutures[i] = threadsWorker->asyncSubmit([this, i] {
                app->drawFrame(currentFrameInfo, i, &currentFrameInfo->drawResults[i]);
            });
        }
        // we must wait for all draw calls to finish
        for (int i = 0; i < appDrawCalls; i++) {
            workerFutures[i].wait();
        }
    }
    // app work is done, we should have a bunch of uncommitted command buffers or a finished image
    currentFrameInfo->numCommandBuffers = currentFrameInfo->countCommandBuffers();
}

void ShadedPathEngine::postFrame()
{
    if (!isDrawResult(currentFrameInfo)) {
        util.warn("Application did not provide any draw result");
        return;
    }
    if (singleThreadMode) {
        ThemedTimer::getInstance()->stop(TIMER_DRAW_FRAME);
        singleThreadPostFrame();
    } else {
        //Error("Multi thread mode not implemented");
        // do the same in multi thread mode (for now)
        ThemedTimer::getInstance()->stop(TIMER_DRAW_FRAME);
        //singleThreadPostFrame();
        // we either have cmd buffers to submit or an already created image
        if (currentFrameInfo->renderedImage != nullptr && currentFrameInfo->renderedImage->rendered == true) {
            //Log("postFrame consume rendered image " << currentFrameInfo->frameNum << endl);
            singleThreadPostFrame();
        } else {
            globalRendering.postFrame(currentFrameInfo);
        }
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
    imageConsumer->consume(currentFrameInfo);
}

bool ShadedPathEngine::isDrawResult(FrameResources* fi)
{
    if (isDrawResultCommandBuffers(fi)) {
        return true;
    }
    if (isDrawResultImage(fi)) {
        return true;
    }
    return false;
}

bool ShadedPathEngine::isDrawResultImage(FrameResources* fi)
{
    if (fi->renderedImage != nullptr && fi->renderedImage->rendered == true) {
        return true;
    }
    return false;
}

bool ShadedPathEngine::isDrawResultCommandBuffers(FrameResources* fi)
{
    if (fi->numCommandBuffers > 0) {
        return true;
    }
    return false;
}

void ShadedPathEngine::waitUntilShutdown()
{
    if (!singleThreadMode) {
        //queue.shutdown();
        threadsMain.join_all();
        if (threadsWorker) {
            delete threadsWorker;
            threadsWorker = nullptr;
        }
        //threadsWorker->join_all();
    }
}

long ShadedPathEngine::getNextFrameNumber()
{
    long n = ++nextFreeFrameNum;
    return n;
}

void ShadedPathEngine::enablePresentation(WindowInfo* winfo)
{
    windowInfos.push_back(winfo);
    
}

void ShadedPathEngine::enableWindowOutput(WindowInfo* winfo)
{
    presentation.preparePresentation(winfo);
}

void ShadedPathEngine::runQueueSubmit(ShadedPathEngine* engine_instance)
{
    LogF("run QueueSubmit start " << endl);
    //pipeline_instance->setRunning(true);
    while (engine_instance->shouldClose() == false) {
        //engine_instance->presentation.beginPresentFrame();
        auto v = engine_instance->queue.pop();
        if (v == nullptr) {
            LogF("engine shutdown" << endl);
            break;
        }
        LogCondF(LOG_QUEUE, "engine received frame: " << v->frameInfo->frameNum << endl);
        if (engine_instance->isDrawResultImage(v->frameInfo)) {
            if (v->frameInfo->renderedImage->rendered == true) {
                // consume rendered image
                Log("submit thread consuming frame image " << v->frameInfo->frameNum << endl);
                if (engine_instance->imageConsumer == nullptr) {
                    Log("WARNING: No image consumer set, defaulting to discarding image\n");
                    engine_instance->imageConsumer = &engine_instance->imageConsumerNullify;
                }
                engine_instance->imageConsumer->consume(v->frameInfo);
            }
        } else if (engine_instance->isDrawResultCommandBuffers(v->frameInfo)) {
            // submit command buffers
            //Log("submit thread submitting frame " << v->frameInfo->frameNum << endl);
            engine_instance->globalRendering.submit(v->frameInfo);
        }
        //engine_instance->shaders.queueSubmit(*v);
        // if we are pop()ed by drawing thread we can be sure to own the thread until presentFence is signalled,
        // we still have to wait for inFlightFence to make sure rendering has ended
        ThemedTimer::getInstance()->start(TIMER_PART_BACKBUFFER_COPY_AND_PRESENT);
        //engine_instance->presentation.presentBackBufferImage(*v); // TODO: test discarding out-of-sync frames
        //engine_instance->advanceFrameCountersAfterPresentation();

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
    engine_instance->queueThreadFinished = true;
}

void ShadedPathEngine::runDrawFrame(ShadedPathEngine* engine_instance)
{
    LogCondF(LOG_QUEUE, "run DrawFrame start " << endl);
    //this_thread::sleep_for(chrono::milliseconds(1000 * (10 - tr->frameIndex)));
    GlobalResourceSet set;
    bool doSwitch;
    ShaderBase* shaderInstance = nullptr;
    //if (tr->frameIndex > 0) {
    //    this_thread::sleep_for(chrono::seconds(3));
    //}
    while (engine_instance->shouldClose() == false) {
        // wait until queue submit thread issued all present commands
        // wait until queue submit thread issued all present commands
        // tr->renderThreadContinue->wait(false);
        optional<unsigned long> o = engine_instance->qsr.renderThreadContinueQueue.pop();
        if (!o) {
            break;
        }
        // draw next frame
        //engine_instance->queueSubmitThreadPreFrame(*tr);
        //engine_instance->drawFrame(*tr);
        //engine_instance->globalUpdate.doSyncedDrawingThreadMaintenance();
        //engine_instance->queue.push(tr);
        engine_instance->preFrame();
        engine_instance->qsr.frameInfo = engine_instance->currentFrameInfo;
        engine_instance->drawFrame();
        engine_instance->postFrame();
        engine_instance->queue.push(&engine_instance->qsr);

        LogCondF(LOG_QUEUE, "pushed frame: " << endl);

    }
    LogCondF(LOG_QUEUE, "run DrawFrame end " << endl);
    //tr->threadFinished = true;
}

