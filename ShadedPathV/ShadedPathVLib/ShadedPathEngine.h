#pragma once

// Engine contains options and aggregates GlobalRendering, Presentation, Shaders, ThreadResources
// who do the vulkan work
class ShadedPathEngine
{
public:
    // construct engine instance together with its needed aggregates
    ShadedPathEngine() :
        global(*this),
        presentation(*this),
        shaders(*this),
        limiter(60.0f)
    {
        Log("Engine c'tor\n");
    }

    virtual ~ShadedPathEngine();

    enum class Resolution { FourK, TwoK, OneK, DeviceDefault, Small };

    // backbuffer sizing
    void setBackBufferResolution(VkExtent2D e);
    void setBackBufferResolution(ShadedPathEngine::Resolution r);
    VkExtent2D getExtentForResolution(ShadedPathEngine::Resolution r);
    VkExtent2D getBackBufferExtent();

    // prevent copy and assigment
    ShadedPathEngine(ShadedPathEngine const&) = delete;
    void operator=(ShadedPathEngine const&) = delete;

    // enable output window, withour calling this only background processing is possible
    void enablePresentation(int w, int h, const char* name);

    // set number of frames that can be worked on in parallel
    void setFramesInFlight(int n);

    int getFramesInFlight() {
        return framesInFlight;
    }

    // limit number of rendered frames - cannot be used together with presentation enabled
    void setFrameCountLimit(long max);

    // default is multi thread mode - use this for all in one single thread
    void setThreadModeSingle() {
        threadModeSingle = true;
    };

    // initialize Vulkan and other libraries, also internal lists and instances
    // no config methods after calling init
    void init();

    // called once to setup commandbuffers for the shaders
    // has to be called after all shaders have been initialized
    void prepareDrawing();

    // call render code in shaders for one frame
    void drawFrame();
    // possibly multi-threaded draw command
    void drawFrame(ThreadResources &tr);

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

    GlobalRendering global;
    Presentation presentation;
    Shaders shaders;
    vector<ThreadResources> threadResources;

    // non-Vulkan members
    Files files;
    GameTime gameTime;
    // presentation
    int win_width = 0;
    int win_height = 0;
    const char* win_name = nullptr;
    bool threadModeSingle = false;
private:
    UI ui;
    long limitFrameCount = 0;
    int framesInFlight = 2;
    bool limitFrameCountEnabled = false;
    bool initialized = false;
    bool threadsAreFinished();
    // backbuffer size:
    VkExtent2D backBufferExtent = getExtentForResolution(Resolution::Small);

    // thread support:
    ThreadGroup threads;
    RenderQueue queue;
    static bool queueThreadFinished;
    void startRenderThreads();
    void startQueueSubmitThread();
    // start the processing thread in the background and return immediately. May only be called once
    static void runDrawFrame(ShadedPathEngine* engine_instance, ThreadResources* tr);
    static void runQueueSubmit(ShadedPathEngine* engine_instance);
    atomic<bool> shutdown_mode = false;
    ThreadLimiter limiter;

    // advance currentFrameIndex and frameNum
    void advanceFrameCountersAfterPresentation();
    // get next frame number for drawing threads:
    long getNextFrameNumber();
    // current frame index - always within 0 .. threadResources.size() - 1
    atomic<size_t> currentFrameIndex = 0;

    // count all frames
    atomic<long> frameNum = 0;
    // for rendering threads
    atomic<long> nextFreeFrameNum = 0;
};

