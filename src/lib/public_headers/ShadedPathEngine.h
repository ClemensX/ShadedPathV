// main engine

#pragma once

// all applications must implement this class and register with engine.
// All callback methods are defined here
class ShadedPathApplication : public EngineParticipant
{
public:
    // called from multiple threads, only local resources should be changed
    //virtual void drawFrame(ThreadResources& tr) = 0;

    // called form main thread, synchronized, for things like window mgmt.
    // be very brief here, as this will block the main thread
    virtual void mainThreadHook() {};
    virtual void run(ContinuationInfo* cont = nullptr) {};
    virtual void handleInput(InputState& inputState) {};
    virtual void backgroundWork() {};
    virtual void prepareFrame(FrameResources* fi) {};
    // draw Frame depending on topic. is in range 0..appDrawCalls-1
    // each topic will be called in parallel threads
    virtual void drawFrame(FrameResources* fi, int topic, DrawResult* drawResult) {};
    // post rendering: just before queue submit
    virtual void postFrame(FrameResources* fi) {};
    // process finished frame: dump to file, copy to window, etc.
    virtual void processImage(FrameResources* fi) {};
    virtual void buildCustomUI() {};
    virtual bool shouldClose() { return true; };
protected:
    double old_seconds = 0.0f;
};

class ShadedPathEngine
{
public:
    ShadedPathEngine() :
        globalRendering(this),
        presentation(this),
        threadsMain(0),
        shaders(this),
        util(this),
        vr(this),
        objectStore(&meshStore),
        sound(*this),
        limiter(60.0f)
    {
        Log("Engine c'tor\n");
#if defined (USE_FIXED_PHYSICAL_DEVICE_INDEX)
        Log("override device selection to device " << PHYSICAL_DEVICE_INDEX << std::endl);
        setFixedPhysicalDeviceIndex(PHYSICAL_DEVICE_INDEX);
#endif
        files.findFxFolder();
    }

    // Prevent copy and assignment
    ShadedPathEngine(ShadedPathEngine const&) = delete;
    void operator=(ShadedPathEngine const&) = delete;

    virtual ~ShadedPathEngine();

    // chain setters
    ShadedPathEngine& setEnableLines(bool enable) { fii(); enableLines = enable; return *this; }
    ShadedPathEngine& setEnableUI(bool enable) { fii(); enableUI = enable; return *this; }
    // enable VR mode. Needs needs active HMD device to work.
    // If no device available will revert back to Stereo mode
    ShadedPathEngine& setVR(bool enable) { fii(); vrMode = enable; if (enable) stereoMode = true; return *this; }
    // exit if VR could not be enabled (instead of reverting to stereo mode)
    ShadedPathEngine& failIfNoVR(bool enable) { fii(); vrEnforce = enable; return *this; }
    // enable stereo mode: 2 render buffers for each shader, 
    // 2 images for left and right eye will be displayed in main window.
    // auto-enabled in VR mode
    ShadedPathEngine& setStereo(bool enable) { fii(); stereoMode = enable; return *this; }
    ShadedPathEngine& setEnableSound(bool enable) { fii(); enableSound = enable; return *this; }
    // default is multi thread mode - use this for all in one single thread
    // will disable render threads and global update thread
    ShadedPathEngine& setSingleThreadMode(bool enable) { fii(); singleThreadMode = enable; return *this; }
    ShadedPathEngine& setDebugWindowPosition(bool enable) { fii(); debugWindowPosition = enable; return *this; }
    ShadedPathEngine& setEnableRenderDoc(bool enable) { fii(); enableRenderDoc = enable; return *this; }
    ShadedPathEngine& setImageConsumer(ImageConsumer* c) { imageConsumer = c; return *this; }
    // how many draw calls should be done in parallel. App will be called with topic counter in range 0..appDrawCalls-1 to be able to separate the work
    ShadedPathEngine& configureParallelAppDrawCalls(int num) { fii(); appDrawCalls = num; return *this; }
    ShadedPathEngine& overrideCPUCores(int usedCores) { fii(); overrideUsedCores = usedCores; return *this; }
    ShadedPathEngine& setContinuationInfo(ContinuationInfo* cont) { continuationInfo = cont; return *this; }
    // enable stereo presentation mode that shows left and right eye output in main window
    ShadedPathEngine& enableStereoPresentation() { fii(); if (isStereo()) { stereoPresentation = true; } return *this; }


    // getters
    bool isDebugWindowPosition() { return debugWindowPosition; }
    bool isSingleThreadMode() { return singleThreadMode; }
    ContinuationInfo* getContinuationInfo() { return continuationInfo; }
    int getParallelAppDrawCalls() { return appDrawCalls; }
    bool isEnableUI() { return enableUI; }
    bool isEnforceVR() { return vrEnforce; }
    bool isSoundEnabled() { return enableSound; }

    bool isMainThread();
    void log_current_thread();
    ThreadInfo mainThreadInfo;

    const std::string engineName = "ShadedPathV";
    const std::string engineVersion = "0.1";
    const uint32_t engineVersionInt = 1;
    std::string vulkanAPIVersion; // = global.getVulkanAPIString();

    enum class Resolution { FourK, TwoK, OneK, DeviceDefault, Small, Invalid, HMD_Native };

    ShadedPathApplication* app = nullptr;
    void registerApp(ShadedPathApplication* app) {
        this->app = app;
        app->setEngine(this);
    }
    ShadedPathApplication* getApp() {
        return app;
    }
    std::string appname = "ShadedPath Engine";

    // run the main loop, called from app
    void eventLoop();

    // backbuffer sizing
    void setBackBufferResolution(VkExtent2D e);
    void setBackBufferResolution(ShadedPathEngine::Resolution r);
    VkExtent2D getExtentForResolution(ShadedPathEngine::Resolution r);
    VkExtent2D getBackBufferExtent();

    // enable output window, can be called any time. Also needed for keyboard input
    void enablePresentation(WindowInfo* winfo);
    // enable Vulkan output to window
    void enableWindowOutput(WindowInfo* winfo);

    // other getters and setters

    bool isVR() {
        return vrMode;
    }

    bool isStereo() {
        return stereoMode;
    }

    bool isStereoPresentation() {
        return stereoPresentation;
    }

    // enable mesh shaders. Will fail to create vulkan device if no suitable GPU is found
    //void enableMeshShader() {
    //    meshShaderEnabled = true;
    //}

    bool isMeshShading() {
        return false; //TODO remove
    }

    void enableKeyEvents() {
        enabledKeyEvents = true;
    }

    void enableMouseMoveEvents() {
        enabledMouseMoveEvents = true;
    }

    void enableMousButtonEvents() {
        enabledMousButtonEvents = true;
    }

    // application should set this in init() for any shader that needs world info
    void setWorld(World* world) {
        this->world = world;
    }

    // if app did not set world, we return nullptr
    World* getWorld() {
        if (world == nullptr) Error("app did not set engine world reference. Use engine->setWorld() in init()");
        return world;
    }

    ////ThreadInfo mainThreadInfo;

    // limit number of rendered frames - cannot be used together with presentation enabled
    void setFrameCountLimit(long max);

    // single queue mode is set automatically, if only one vulkan queue is available 
    // on the device. Will cause severe perfomance penalties
    void setSingleQueueMode() {
        singleQueueMode = true;
    }

    bool isSingleQueueMode() {
        return this->singleQueueMode;
    }

    // applications have to set max number of used textures because we need to know
    // at shader creation time
    // default value is 5
    ShadedPathEngine& setMaxTextures(size_t maxTextures) {
        fii();
        this->maxTextures = maxTextures;
        return *this;
    }

    void setFixedPhysicalDeviceIndex(int i) {
        fixedPhysicalDeviceIndex = i;
    }

    // if returned value is >= 0, the engine will use this device index for vulkan device creation
    int getFixedPhysicalDeviceIndex() {
        return fixedPhysicalDeviceIndex;
    }

    // called once to setup commandbuffers for the shaders
    // has to be called after all shaders have been initialized
    void prepareDrawing();

    int getNumCores() {
        return numCores;
    }

    ThreadGroup* getWorkerThreads() {
        return threadsWorker;
    }

    // init global resources. will only be available once. Many engine parameters
    // are not allowed to change after this call
    void initGlobal(std::string appname = "");

    GlobalRendering globalRendering;
    Shaders shaders;
    Util util;
    VR vr;
private:
    // need to insert here for proper destruction order
    std::array<FrameResources, 2> frameInfos; // only 2 frame infos needed for alternating during draw calls, initialized in initGlobal()
public:
    TextureStore textureStore;
    MeshStore meshStore;
    WorldObjectStore objectStore;
    Sound sound;

    // non-Vulkan members
    Files files;
    GameTime gameTime;
    FPSCounter fpsCounter;
    UI ui;
    // get aspect ratio of backbuffer - window should be same, but that is not enforced
    float getAspect() {
        return backBufferAspect;
    }
    // create image in backbuffer size
    GPUImage* createImage(const char* debugName);
    Presentation presentation;
    bool presentationMode = true; // get rid of this later
    int numWorkerThreads = 0;
    int getFramesInFlight() {
        return numWorkerThreads; // 1 for  single thread mode, 2 or more for multi
    }
    std::array<FrameResources,2>& getFrameResources() {
        return frameInfos;
    }
    // unsafe check if background thread is available - you still must do a reservation
    bool isBackgroundThreadAvailable() {
        return backgroundThreadAvailable;
    }
    // reserve background thread. If false is returned the thread was not available
    // backgroundWork() in app code may be immediately called, even before this method returns,
    // so make sure all is prepared before making a reservation
    bool reserveBackgroundThread() {
        if (!backgroundThreadAvailable) return false;
        backgroundThreadAvailable = false;
        backgroundThreadQueue.push(&backRes);
        return true;
    }
private:

    // bool configuration flags:
    bool enableLines = false;
    bool enableUI = false;
    bool enableSound = false;
    bool singleThreadMode = false;
    bool debugWindowPosition = false; // if true try to open app window in right screen part
    bool enableRenderDoc = true;
    bool stereoMode = false;
    ImageConsumer* imageConsumer = nullptr;
    ImageConsumerNullify imageConsumerNullify;

    // backbuffer size:
    VkExtent2D backBufferExtent = getExtentForResolution(Resolution::Small);
    float backBufferAspect = 1.0f;
    long limitFrameCount = 0;
    size_t maxTextures = 5;
    bool limitFrameCountEnabled = false;
    bool initialized = false;
    bool threadsAreFinished();
    bool enabledKeyEvents = false;
    bool enabledMouseMoveEvents = false;
    bool enabledMousButtonEvents = false;
    bool vrMode = false;
    bool vrEnforce = false;
    bool stereoPresentation = false;
    //bool meshShaderEnabled = false;
    bool singleQueueMode = false;
    int fixedPhysicalDeviceIndex = -1;
    World* world = nullptr;
    std::vector<GPUImage> images;
    // thread support:
    // main threads for global things like QueueSubmit thread and background thread
    ThreadGroup threadsMain;
    // worker threads for rendering and other activities during frame generation
    ThreadGroup* threadsWorker = nullptr;
    RenderQueue queue;
    RenderQueue backgroundThreadQueue;
    std::atomic<bool> backgroundThreadAvailable = true;
    bool queueThreadFinished = false;
    QueueSubmitResources qsr;
    QueueSubmitResources backRes;

    //std::future<void>* workerFutures = nullptr;
    std::vector<std::future<void>> workerFutures;
    ThreadGroup& getThreadGroupMain() {
        return threadsMain;
    }
    // check if engine should close
    bool shouldClose();
    // increase frame number and prepare frame info, advance timer, wait for old frame with same index to have finished
    void preFrame();
    // draw the frame (create command buffers) from multiple threads
    void drawFrame();
    // clean up draw and prepare submit, single threaded
    void postFrame();
    void waitUntilShutdown();
    // command buffers need to have been already counted before calling this
    bool isDrawResult(FrameResources* fi);
    // command buffers need to have been already counted before calling this
    bool isDrawResultImage(FrameResources* fi);
    // command buffers need to have been already counted before calling this
    bool isDrawResultCommandBuffers(FrameResources* fi);
    // we no longer need frame num to be atomic
    //std::atomic<long> nextFreeFrameNum = 0;
    long nextFreeFrameNum = 0;
    long getNextFrameNumber();
    // beware! current draw frame info is only valid during frame creation (preFrame() to postFrame())
    FrameResources* currentFrameInfo = nullptr;
    // in single thread mode handle post processing (consume image, advance sound, etc.)
    void singleThreadPostFrame();
    void initFrame(FrameResources* fi, long frameNum);
    int numCores = 0;
    int overrideUsedCores = -1;
    int appDrawCalls = 1;
    static void runDrawFrame(ShadedPathEngine* engine_instance);
    // in queue submit thread we submit the command buffers of the current frame,
    // this should take some time time to process, so we display the last frame in the meantime
    // basically we are 1 frame behind with rendering
    // for VR this doesn't work, because 1 frame behind leads to display smearing: our drawings lag behind HMD movement
    // VR system drawings (like SteamVR desk) are ok, so for debugging we can see different movement of e.g. our drawings and the SteamVR desk
    static void runQueueSubmit(ShadedPathEngine* engine_instance);
    static void runUpdateThread(ShadedPathEngine* engine_instance);
    ThreadLimiter limiter;
    void startRenderThread();
    void startQueueSubmitThread();
    // global update thread for shuffling data to GPU in the background
    void startUpdateThread();
    std::vector<WindowInfo*> windowInfos;
    ContinuationInfo* continuationInfo = nullptr;
    // fail if initialized. Util method for checking failing if engine is already initialized
    void fii() {
        if (initialized) {
            Error("Engine already initialized. Cannot change this parameter after initialization\n");
        }
    }
};