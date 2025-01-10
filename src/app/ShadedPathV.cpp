// ShadedPathV.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "mainheader.h"
#include "SimpleMultiApp.h"
#include "SimpleMultiWin.h"
//#include "AppSupport.h"
//#include "LandscapeGenerator.h"
//#include "SimpleApp.h"
//#include "DeviceCoordApp.h"
//#include "LineApp.h"
//#include "gltfObjectsApp.h"
//#include "gltfTerrainApp.h"
//#include "GeneratedTexturesApp.h"
//#include "BillboardDemo.h"
//#include "TextureViewer.h"
//#include "LandscapeDemo1.h"
//#include "incoming.h"

///////////////////
//#include <GLFW/glfw3.h>
//#include <iostream>

//#define TEST_APP 1

#if defined(TEST_APP)
const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class VulkanApp {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    VkInstance instance;

    void initWindow() {
        glfwSetErrorCallback(glfwErrorCallback);

        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        //glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Window", nullptr, nullptr);
        if (!window) {
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwSetKeyCallback(window, keyCallback);
    }

    void initVulkan() {
        createInstance();
    }

    void createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Vulkan";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        createInfo.enabledExtensionCount = 0;// glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensions;

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance");
        }
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    static void glfwErrorCallback(int error, const char* description) {
        std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
    }

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            std::cout << "Key pressed: " << key << std::endl;
        }
        else if (action == GLFW_RELEASE) {
            std::cout << "Key released: " << key << std::endl;
        }

        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }
};

int main() {
    VulkanApp app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
///////////////////
#else

int main()
{
    // TODO: investigate options to have multiple render apps.
    //       in the end we want to have a single app that can switch between different render apps.
    //       and it should use the same glfw window...
    //       start with copilot prompt: are the static members of my classes preventing me from using multiple instances of ShadedPathEngine?

    Log("ShadedPathV app\n");
    ShadedPathEngine engine;
    engine
        .setEnableLines(true)
        .setDebugWindowPosition(true)
        .setEnableUI(true)
        .setEnableSound(true)
        .setVR(false)
        //.setSingleThreadMode(true)
        .overrideCPUCores(4)
        ;


    //engine.setFixedPhysicalDeviceIndex(0);
    engine.initGlobal();
    SimpleMultiApp app;
    SimpleMultiApp2 app2;
    ContinuationInfo cont;
    engine.registerApp((ShadedPathApplication*)&app);
    engine.app->run(&cont);
    if (cont.cont) {
        ShadedPathEngine engine;
        engine
            .setEnableLines(true)
            .setDebugWindowPosition(true)
            .setEnableUI(true)
            .setEnableSound(true)
            .setVR(false)
            //.setSingleThreadMode(true)
            .overrideCPUCores(4)
            ;
        engine.initGlobal();
        engine.registerApp((ShadedPathApplication*)&app2);
        // uncomment for chained app execution:
        cont.cont = false;
        engine.app->run(&cont);

    }
    return 0;
}






    //ShadedPathEngineManager man;
    //ShadedPathEngine* engine = nullptr;
    //ShadedPathEngine* oldEngine = nullptr;
    //TextureViewer app; // vr ok
    //TextureViewer* oldApp = nullptr;
    //{
    //    engine = man.createEngine();
    //    //Incoming app;
    //    //gltfTerrainApp app; // vr ok
    //    //LineApp app; // vr ok
    //    //SimpleApp app; // vr ok (some stuttering - will not be investigated)
    //    //DeviceCoordApp app; // vr not supported
    //    //BillboardDemo app; // vr ok
    //    //GeneratedTexturesApp app; // TODO: does not even work in 2D
    //    //gltfObjectsApp app; // vr ok, also skybox
    //    //LandscapeDemo app; // vr ok
    //    //LandscapeGenerator app; // vr ok with limited support
    //    app.setEngine(engine);
    //    app.run();
    //    //man.deleteEngine(engine);
    //    oldApp = &app;
    //    oldEngine = engine;
    //}
    //if (true && !oldEngine->shouldClosePermanent)
    //{
    //    //man.deleteEngine(oldEngine); // delete old window, hav to create a new one
    //    engine = man.addEngineInApplicationWindow(oldEngine, oldApp);
    //    //man.deleteEngine(oldEngine);
    //    Incoming app;
    //    app.setEngine(engine);
    //    app.run();
    //    man.deleteEngine(engine);
    //}
    //if (false)
    //{
    //    engine = man.createEngine();
    //    Incoming app;
    //    app.setEngine(engine);
    //    app.run();
    //    man.deleteEngine(engine);
    //}

#endif
