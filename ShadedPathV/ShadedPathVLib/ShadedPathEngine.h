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
        shaders(*this)
    {
        Log("Engine c'tor\n");
    }

    virtual ~ShadedPathEngine()
    {
        Log("Engine destructor\n");
        if (global.device) vkDeviceWaitIdle(global.device);
        ThemedTimer::getInstance()->logInfo("DrawFrame");
        ThemedTimer::getInstance()->logFPS("DrawFrame");
    };

    enum class Resolution { FourK, TwoK, OneK, DeviceDefault, Small };

    // backbuffer sizing
    void setBackBufferResolution(VkExtent2D e);
    void setBackBufferResolution(ShadedPathEngine::Resolution r);
    VkExtent2D getExtentForResolution(ShadedPathEngine::Resolution r);
    VkExtent2D getBackBufferExtent();

    // prevent copy and assigment
    //ShadedPathEngine(ShadedPathEngine const&) = delete;
    //void operator=(ShadedPathEngine const&) = delete;

    // enable output window, withour calling this only background processing is possible
    void enablePresentation(int w, int h, const char* name);

    // set number of frames that can be worked on in parallel
    void setFramesInFlight(int n);

    int getFramesInFlight() {
        return framesInFlight;
    }

    void setFrameCountLimit(long max);

    // initialize Vulkan and other libraries, also internal lists and instances
    // no config methods after calling init
    void init();

    // called once to setup commandbuffers for the shaders
    // has to be called after all shaders have been initialized
    void prepareDrawing();

    // call render code in shaders for one frame
    void drawFrame();

    // poll events via presentation layer
    void pollEvents();

    // check if engine should shutdown.
    // if presenting glfw will tell us window was closed
    // if background processing some other threshold like max number of frames might have been reached
    bool shouldClose();

    // current frame index - always within 0 .. threadResources.size() - 1
    size_t currentFrameIndex = 0;

    // count all frames
    long frameNum = 0;

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
private:
    long limitFrameCount = 0;
    int framesInFlight = 2;
    bool limitFrameCountEnabled = false;
    bool initialized = false;
    // exit Vulkan and free resources
    void shutdown();

    // backbuffer size:
    VkExtent2D backBufferExtent = getExtentForResolution(Resolution::Small);

};

