// main engine

#pragma once

// all applications must implement this class and register with engine.
// All callback methods are defined here
class ShadedPathApplication
{
public:
    // called from multiple threads, only local resources should be changed
    //virtual void drawFrame(ThreadResources& tr) = 0;

    // called form main thread, synchronized, for things like window mgmt.
    // be very brief here, as this will block the main thread
    virtual void mainThreadHook() {};
    virtual void handleInput(InputState& inputState) {};
    virtual void prepareFrame(FrameInfo* fi) {};
    // draw Frame depending on topic. is in range 0..appDrawCalls-1
    // each topic will be called in parallel threads
    virtual void drawFrame(FrameInfo* fi, int topic) {};
    virtual void buildCustomUI() {};
    virtual bool shouldClose() { return true; };
    virtual void run() {};
    void registerEngine(ShadedPathEngine* engine) {
        this->engine = engine;
    }
protected:
    double old_seconds = 0.0f;
    ShadedPathEngine* engine = nullptr;
};

// most simple image consumer: just discard the image
class ImageConsumerNullify : public ImageConsumer
{
public:
    void consume(FrameInfo* fi) override {
        fi->renderedImage->consumed = true;
        fi->renderedImage->rendered = false;
    }
};

// image consumer to dump generated images to disk
class ImageConsumerDump : public ImageConsumer
{
public:
    void consume(FrameInfo* fi) override;
    void configureFramesToDump(bool dumpAll, std::initializer_list<long> frameNumbers);
    ImageConsumerDump(ShadedPathEngine* s) {
        setEngine(s);
        directImage.setEngine(s);
    }
private:
    bool dumpAll = false;
    std::unordered_set<long> frameNumbersToDump;
    DirectImage directImage;
};

class ShadedPathEngine
{
public:
    ShadedPathEngine() :
        globalRendering(this),
        presentation(this),
        threadsMain(0),
        //shaders(*this),
        util(this)
        //vr(this)
    {
        Log("Engine c'tor\n");
        //files.findFxFolder();
    }

    // Prevent copy and assignment
    ShadedPathEngine(ShadedPathEngine const&) = delete;
    void operator=(ShadedPathEngine const&) = delete;

    virtual ~ShadedPathEngine();

    // chain setters
    ShadedPathEngine& setEnableLines(bool enable) { enableLines = enable; return *this; }
    ShadedPathEngine& setEnableUI(bool enable) { enableUI = enable; return *this; }
    ShadedPathEngine& setVR(bool enable) { enableVr = enable; return *this; }
    ShadedPathEngine& setStereo(bool enable) { enableStereo = enable; return *this; }
    ShadedPathEngine& setEnableSound(bool enable) { enableSound = enable; return *this; }
    // default is multi thread mode - use this for all in one single thread
    // will disable render threads and global update thread
    ShadedPathEngine& setSingleThreadMode(bool enable) { singleThreadMode = enable; return *this; }
    ShadedPathEngine& setDebugWindowPosition(bool enable) { debugWindowPosition = enable; return *this; }
    ShadedPathEngine& setEnableRenderDoc(bool enable) { enableRenderDoc = enable; return *this; }
    ShadedPathEngine& setImageConsumer(ImageConsumer* c) { imageConsumer = c; return *this; }
    // how many draw calls should be done in parallel. App will be called with topic counter in range 0..appDrawCalls-1 to be able to separate the work
    ShadedPathEngine& configureParallelAppDrawCalls(int num) { appDrawCalls = num; return *this; }
    ShadedPathEngine& overrideCPUCores(int usedCores) { overrideUsedCores = usedCores; return *this; }

    // getters
    bool isDebugWindowPosition() { return debugWindowPosition; }
    bool isSingleThreadMode() { return singleThreadMode; }

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
        app->registerEngine(this);
    }
    ShadedPathApplication* getApp() {
        return app;
    }

    // run the main loop, called from app
    void eventLoop();

    // backbuffer sizing
    void setBackBufferResolution(VkExtent2D e);
    void setBackBufferResolution(ShadedPathEngine::Resolution r);
    VkExtent2D getExtentForResolution(ShadedPathEngine::Resolution r);
    VkExtent2D getBackBufferExtent();

    // enable output window, can be called any time. Also needed for keyboard input
    void enablePresentation(WindowInfo* winfo, int w, int h, const char* name);
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
    void enableMeshShader() {
        meshShaderEnabled = true;
    }

    bool isMeshShading() {
        return meshShaderEnabled;
    }

    // set number of frames that can be worked on in parallel
    void setFramesInFlight(int n);

    int getFramesInFlight() {
        return framesInFlight;
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

    //// application should set this in init() for any shader that needs world info
    //void setWorld(World* world) {
    //    this->world = world;
    //}

    //// if app did not set world, we return nullptr
    //World* getWorld() {
    //    return world;
    //}

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

    // init global resources. will only be available once
    void initGlobal();
    GlobalRendering globalRendering;
    Util util;
    //Shaders shaders;
    //VR vr;
    //TextureStore textureStore;

    // non-Vulkan members
    //Files files;
    GameTime gameTime;
    FPSCounter fpsCounter;
    // create image in backbuffer size
    GPUImage* createImage(const char* debugName);
    bool presentationMode = true; // get rid of this later

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
    int framesInFlight = 2;
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
    bool meshShaderEnabled = false;
    bool soundEnabled = false;
    bool singleQueueMode = false;
    int fixedPhysicalDeviceIndex = -1;
    World* world = nullptr;
    std::vector<GPUImage> images;
    // thread support:
    // main threads for global thinkgs like QueueSubmit thread
    ThreadGroup threadsMain;
    // worker threads for rendering and other activities during frame generation
    ThreadGroup* threadsWorker = nullptr;
    //std::future<void>* workerFutures = nullptr;
    std::vector<std::future<void>> workerFutures;
    ThreadGroup& getThreadGroupMain() {
        return threadsMain;
    }
    bool shouldClose();
    void preFrame();
    void drawFrame();
    void postFrame();
    void waitUntilShutdown();
    // we no longer need frame num to be atomic
    //std::atomic<long> nextFreeFrameNum = 0;
    long nextFreeFrameNum = 0;
    long getNextFrameNumber();
    // beware! current draw frame info is only valid during frame creation (preFrame() to postFrame())
    FrameInfo* currentFrameInfo = nullptr;
    FrameInfo frameInfos[2];
    // in single thread mode handle post processing (consume image, advance sound, etc.)
    void singleThreadPostFrame();
    void initFrame(FrameInfo* fi, long frameNum);
    int numCores = 0;
    int overrideUsedCores = -1;
    int appDrawCalls = 1;
    Presentation presentation;
    static void runDrawFrame(ShadedPathEngine* engine_instance);
    static void runQueueSubmit(ShadedPathEngine* engine_instance);
    static void runUpdateThread(ShadedPathEngine* engine_instance);
    void startRenderThread();
    void startQueueSubmitThread();
    // global update thread for shuffling data to GPU in the background
    void startUpdateThread();
    std::vector<WindowInfo*> windowInfos;
};