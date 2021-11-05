#include "pch.h"


void SimpleApp::run()
{
    Log("SimpleApp started" << endl);
    {
        ShadedPathEngine engine;
        engine.enablePresentation(800, 600, "Vulkan Simple App");
        engine.init();
        engine.global.initiateShader_Triangle();
        while (!glfwWindowShouldClose(engine.window)) {
            glfwPollEvents();
        }
        engine.shutdown();
    }
    Log("SimpleApp ended" << endl);
}
