// main engine

#pragma once

class ShadedPathEngine
{
public:
    ShadedPathEngine() :
        global(this),
        shaders(*this),
        util(this),
        vr(this)
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
    ShadedPathEngine& setSingleThreadMode(bool enable) { singleThreadMode = enable; return *this; }
    ShadedPathEngine& setDebugWindowPosition(bool enable) { debugWindowPosition = enable; return *this; }
    ShadedPathEngine& setEnableRenderDoc(bool enable) { enableRenderDoc = enable; return *this; }

    const std::string engineName = "ShadedPathV";
    const std::string engineVersion = "0.1";
    const uint32_t engineVersionInt = 1;
    std::string vulkanAPIVersion; // = global.getVulkanAPIString();

    enum class Resolution { HMDIndex, FourK, TwoK, OneK, DeviceDefault, Small };

    // backbuffer sizing
    void setBackBufferResolution(VkExtent2D e);
    void setBackBufferResolution(ShadedPathEngine::Resolution r);
    VkExtent2D getExtentForResolution(ShadedPathEngine::Resolution r);
    VkExtent2D getBackBufferExtent();

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

    // application should set this in init() for any shader that needs world info
    void setWorld(World* world) {
        this->world = world;
    }

    // if app did not set world, we return nullptr
    World* getWorld() {
        return world;
    }

    //ThreadInfo mainThreadInfo;

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

    // init global resources. will only be available once
    void initGlobal();
    GlobalRendering global;
    Util util;
    Shaders shaders;
    VR vr;
    TextureStore textureStore;

    // non-Vulkan members
    Files files;
    GameTime gameTime;
    FPSCounter fpsCounter;
    bool threadModeSingle = false;

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

};