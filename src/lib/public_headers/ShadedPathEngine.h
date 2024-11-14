#pragma once

#include "mainheader.h"

// forward declarations
//class ShadedPathEngineManager;
class ShadedPathApplication;
//class World;
//class ThreadResources;
//struct GlobalUpdateElement;
//struct ShaderUpdateElement;
//struct InputState;

// .the engine.
class ShadedPathEngine
{
private:
    // construct engine instance together with its needed aggregates
    ShadedPathEngine() :
        global(*this),
		globalUpdate(*this),
        presentation(*this),
        shaders(*this),
        util(*this),
        vr(*this),
        objectStore(&meshStore),
        sound(*this),
        limiter(60.0f)
    {
        Log("Engine c'tor\n");
        files.findFxFolder();
    }

public:
    // Prevent copy and assignment
    ShadedPathEngine(ShadedPathEngine const&) = delete;
    void operator=(ShadedPathEngine const&) = delete;

    virtual ~ShadedPathEngine();

    // engine state - may be read by apps
    enum State {
        INIT,        // before any rendering, all file loading should be done here
        RENDERING    // in render loop - avoid any unnecessary secondary tasks
    };
    const std::string engineName = "ShadedPathV";
    const std::string engineVersion = "0.1";
    const uint32_t engineVersionInt = 1;
    std::string vulkanAPIVersion; // = global.getVulkanAPIString();

    enum class Resolution { HMDIndex, FourK, TwoK, OneK, DeviceDefault, Small };

    void registerApp(ShadedPathApplication* app) {
        this->app = app;
    }

    // backbuffer sizing
    void setBackBufferResolution(VkExtent2D e);
    void setBackBufferResolution(ShadedPathEngine::Resolution r);
    VkExtent2D getExtentForResolution(ShadedPathEngine::Resolution r);
    VkExtent2D getBackBufferExtent();

    // enable output window, without calling this only background processing is possible
    void enablePresentation(int w, int h, const char* name);

    // enable UI overlay. currently only possible if presentation is enabled
    void enableUI();

    // enable VR mode. Needs needs active HMD device to work.
    // If no device available will revert back to Stereo mode
    void enableVR(bool enable = true) {
        vrMode = enable;
        if (enable) stereoMode = true;
    }

    bool isVR() {
        return vrMode;
    }

    bool isRendering() {
        return state == RENDERING;
    }
    // enable stereo mode: 2 render buffers for each shader, 
    // 2 images for left and right eye will be displayed in main window.
    // auto-enabled in VR mode
    void enableStereo() {
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

    void enableSound(bool enable = true) {
        soundEnabled = enable;
    }

    bool isSoundEnabled() {
        return soundEnabled;
    }

    // application should set this in init() for any shader that needs world info
    void setWorld(World* world) {
        this->world = world;
    }

    // if app did not set world, we return nullptr
    World* getWorld() {
        return world;
    }

    ThreadInfo mainThreadInfo;

    // limit number of rendered frames - cannot be used together with presentation enabled
    void setFrameCountLimit(long max);

    // default is multi thread mode - use this for all in one single thread
    // will disable render threads and global update thread
    void setThreadModeSingle() {
        threadModeSingle = true;
    };

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
    // initialize Vulkan and other libraries, also internal lists and instances
    // no config methods after calling init
    void init(std::string appname);

    // called once to setup commandbuffers for the shaders
    // has to be called after all shaders have been initialized
    void prepareDrawing();

    // call render code in shaders for one frame
    void drawFrame();
    // possibly multi-threaded draw command
    void drawFrame(ThreadResources& tr);
    // some things need to be done from only one thread (like sound updates).
    // Apps should check with this method during updatePerFrame()
    // should be replaced by global update thread later? // TODO
    bool isDedicatedRenderUpdateThread(ThreadResources& tr);

    // poll events via presentation layer
    void pollEvents();

    // check if engine should shutdown.
    // if presenting glfw will tell us window was closed
    // if background processing some other threshold like max number of frames might have been reached
    bool shouldClose();

    // Is engine in shutdown mode? 
    bool isShutdown() { return shutdown_mode; }
    // enable shutdown mode: The run threads will dry out and terminate
    void shutdown();
    // Wait until engine threads have ended rendering.
    void waitUntilShutdown();
    // TODO describe
    void queueSubmitThreadPreFrame(ThreadResources& tr);
    void queueSubmitThreadPostFrame(ThreadResources& tr);

    GlobalRendering global;
	GlobalUpdate globalUpdate;
    Presentation presentation;
    Shaders shaders;
    Util util;
    VR vr;
    std::vector<ThreadResources> threadResources;
    TextureStore textureStore;
    MeshStore meshStore;
    WorldObjectStore objectStore;
    Sound sound;

    // non-Vulkan members
    Files files;
    GameTime gameTime;
    FPSCounter fpsCounter;
    // presentation
    int win_width = 0;
    int win_height = 0;
    const char* win_name = nullptr;
    bool threadModeSingle = false;
    UI ui;
    // get aspect ratio of backbuffer - window should be same, but that is not enforced
    float getAspect() {
        return backBufferAspect;
    }
    ShadedPathApplication* app = nullptr;
    std::string appname;
	auto& getShaderUpdateQueue() {
		return shaderUpdateQueue;
	}
    void pushUpdate(GlobalUpdateElement* updateElement);
    bool isUpdateThread() {
		return threadModeSingle || isUpdateThread_;
	}

    void log_current_thread();
    bool renderThreadDebugLog = false;
    bool debugWindowPosition = false; // app window in right screen part

    // move static fields of the other classes here to enable multi-engine support
    int threadResourcesCount = 0;

private:
    thread_local static bool isUpdateThread_;
    ThreadGroup& getThreadGroup() {
        return threads;
    }
    State state = INIT;
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
    // backbuffer size:
    VkExtent2D backBufferExtent = getExtentForResolution(Resolution::Small);
    // check if backbuffer and window have same aspect - warning if not
    void checkAspect();

    // thread support:
    ThreadGroup threads;
    RenderQueue queue;
    // we simply use indexes into the update array for handling resources
    ThreadsafeWaitingQueue<GlobalUpdateElement*> shaderUpdateQueue;

    // we need a separate update queue for threadModeSingle
    std::queue<ShaderUpdateElement*> shaderUpdateQueueSingle; // TODO: rework
    static bool queueThreadFinished;
    void startRenderThreads();
    void startQueueSubmitThread();
    // global update thread for shuffling data to GPU in the background
    void startUpdateThread();
    // start the processing thread in the background and return immediately. May only be called once
    static void runDrawFrame(ShadedPathEngine* engine_instance, ThreadResources* tr);
    static void runQueueSubmit(ShadedPathEngine* engine_instance);
    static void runUpdateThread(ShadedPathEngine* engine_instance);
    std::atomic<bool> shutdown_mode = false;
    ThreadLimiter limiter;

    // advance currentFrameIndex and frameNum
    void advanceFrameCountersAfterPresentation();
    // get next frame number for drawing threads:
    long getNextFrameNumber();
    // current frame index - always within 0 .. threadResources.size() - 1
    std::atomic<size_t> currentFrameIndex = 0;

    // count all frames
    std::atomic<long> frameNum = 0;
    // for rendering threads
    std::atomic<long> nextFreeFrameNum = 0;

    friend class ShadedPathEngineManager;
};

class ShadedPathEngineManager
{
public:
    // Create a new instance of ShadedPathEngine and store it in the list
    ShadedPathEngine* createEngine()
    {
        auto engine = std::unique_ptr<ShadedPathEngine>(new ShadedPathEngine());
        engines.push_back(std::move(engine));
        return engines.back().get();
    }

    // delete engine instance by pointer
    void deleteEngine(ShadedPathEngine* engine)
    {
        for (auto it = engines.begin(); it != engines.end(); ++it)
        {
            if (it->get() == engine)
            {
                engines.erase(it);
                return;
            }
        }
    }

    // Get an engine instance by index
    ShadedPathEngine* getEngine(size_t index)
    {
        if (index < engines.size())
        {
            return engines[index].get();
        }
        return nullptr;
    }

    // Get the number of engine instances
    size_t getEngineCount() const
    {
        return engines.size();
    }

private:
    std::vector<std::unique_ptr<ShadedPathEngine>> engines;
};

// all applications must implement this class and register with engine.
// All callback methods are defined here
class ShadedPathApplication
{
public:
    // called from multiple threads, only local resources should be changed
    virtual void drawFrame(ThreadResources& tr) = 0;
    virtual void handleInput(InputState& inputState) = 0;
    virtual void buildCustomUI() {};
};
