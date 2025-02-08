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
    virtual void handleInput(InputState& inputState) {};
    virtual void prepareFrame(FrameResources* fi) {};
    // draw Frame depending on topic. is in range 0..appDrawCalls-1
    // each topic will be called in parallel threads
    virtual void drawFrame(FrameResources* fi, int topic, DrawResult* drawResult) {};
    // post rendering: before queue submit (present rendered frame or dump to file, etc.)
    virtual void postFrame(FrameResources* fi) {};
    virtual void buildCustomUI() {};
    virtual bool shouldClose() { return true; };
    virtual void run(ContinuationInfo* cont = nullptr) {};
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
        objectStore(&meshStore)
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
    ShadedPathEngine& setVR(bool enable) { fii(); enableVr = enable; return *this; }
    ShadedPathEngine& setStereo(bool enable) { fii(); enableStereo = enable; return *this; }
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

    // getters
    bool isDebugWindowPosition() { return debugWindowPosition; }
    bool isSingleThreadMode() { return singleThreadMode; }
    ContinuationInfo* getContinuationInfo() { return continuationInfo; }
    int getParrallelAppDrawCalls() { return appDrawCalls; }

    bool isMainThread();
    void log_current_thread();
    ThreadInfo mainThreadInfo;

    const std::string engineName = "ShadedPathV";
    const std::string engineVersion = "0.1";
    const uint32_t engineVersionInt = 1;
    std::string vulkanAPIVersion; // = global.getVulkanAPIString();

    enum class Resolution { FourK, TwoK, OneK, DeviceDefault, Small, Invalid };

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

    // enable VR mode. Needs needs active HMD device to work.
    // If no device available will revert back to Stereo mode
    void enableVR(bool enable = true) {
        vrMode = enable;
        if (enable) stereoMode = true;
    }

    bool isVR() {
        return vrMode;
    }

    // enable stereo mode: 2 render buffers for each shader, 
        // 2 images for left and right eye will be displayed in main window.
        // auto-enabled in VR mode
    void enableStereoMode() {
        stereoMode = true;
    }

    bool isStereo() {
        return stereoMode;
    }

    // enable stereo presentation mode that shows left and right eye output in main window
    void enableStereoPresentation() {
        if (isStereo()) {
            stereoPresentation = true;
        }
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
    void setMaxTextures(size_t maxTextures) {
        this->maxTextures = maxTextures;
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
    Util util;
private:
    // need to insert here for proper destruction order
    std::array<FrameResources, 2> frameInfos; // only 2 frame infos needed for alternating during draw calls, initialized in initGlobal()
public:
    Shaders shaders;
    VR vr;
    TextureStore textureStore;
    MeshStore meshStore;
    WorldObjectStore objectStore;
    //Sound sound;

    // non-Vulkan members
    Files files;
    GameTime gameTime;
    FPSCounter fpsCounter;
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
private:

    // bool configuration flags:
    bool enableLines = false;
    bool enableUI = false;
    bool enableVr = false;
    bool enableStereo = false;
    bool enableSound = false;
    bool singleThreadMode = false;
    bool debugWindowPosition = false; // if true try to open app window in right screen part
    bool enableRenderDoc = true;
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
    bool stereoMode = false;
    bool stereoPresentation = false;
    //bool meshShaderEnabled = false;
    bool soundEnabled = false;
    bool singleQueueMode = false;
    int fixedPhysicalDeviceIndex = -1;
    World* world = nullptr;
    std::vector<GPUImage> images;
    // thread support:
    // main threads for global things like QueueSubmit thread
    ThreadGroup threadsMain;
    // worker threads for rendering and other activities during frame generation
    ThreadGroup* threadsWorker = nullptr;
    RenderQueue queue;
    bool queueThreadFinished = false;
    bool processImageThreadFinished = false;
    QueueSubmitResources qsr;

    //std::future<void>* workerFutures = nullptr;
    std::vector<std::future<void>> workerFutures;
    ThreadGroup& getThreadGroupMain() {
        return threadsMain;
    }
    bool shouldClose();
    void preFrame();
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
    static void runQueueSubmit(ShadedPathEngine* engine_instance);
    static void runProcessImage(ShadedPathEngine* engine_instance);
    static void runUpdateThread(ShadedPathEngine* engine_instance);
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