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
        ThemedTimer::getInstance()->logInfo("DrawFrame");
        ThemedTimer::getInstance()->logFPS("DrawFrame");
    };

    GlobalRendering global;
    Presentation presentation;
    Shaders shaders;
    vector<ThreadResources> threadResources;


    // prevent copy and assigment
    //ShadedPathEngine(ShadedPathEngine const&) = delete;
    //void operator=(ShadedPathEngine const&) = delete;

    // initialize Vulkan
    void init();

    // enable output window, withour calling this only background processing is possible
    void enablePresentation(int w, int h, const char* name) {
        if (false) {
            Error("Changing presentation mode after initialization is not possible!");
        }
        win_width = w;
        win_height = h;
        win_name = name;
        currentExtent.width = w;
        currentExtent.height = h;
        presentationEnabled = true;
    };

    // set number of frames that can be worked on in parallel
    // default is 2
    void setFramesInFlight(int n) {
        framesInFlight = n;
        threadResources.resize(framesInFlight);
    }

    void setFrameCountLimit(long max) {
        limitFrameCount = max;
    }

    // current frame index - always within 0 .. threadResources.size() - 1
    size_t currentFrameIndex = 0;

    // count all frames
    long frameNum = 0;

    // called once to setup commandbuffers for the shaders
    // has to be called after all shaders have been initialized
    void prepareDrawing();

    // call render code in shaders for one frame
    void drawFrame();

    // poll events from glfw
    void pollEvents();

    // check if engine should shutdown.
    // if presenting glfw will tell us window was closed
    // if background processing some other threshold like max number of frames might have been reached
    bool shouldClose();

    // non-Vulkan members
    Files files;
    GameTime gameTime;
    VkExtent2D getCurrentExtent();
private:
    long limitFrameCount = 0;
    int framesInFlight = 2;
    bool presentationEnabled = false;
    // exit Vulkan and free resources
    void shutdown();

    // presentation
    int win_width = 0;
    int win_height = 0;
    const char* win_name = nullptr;
    // if no window or backbuffer size was set by application:
    VkExtent2D defaultExtent = { 500, 400 };
    VkExtent2D currentExtent = defaultExtent;

};

