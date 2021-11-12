#include "pch.h"


void SimpleApp::run()
{
    Log("SimpleApp started" << endl);
    {
        ShadedPathEngine engine;
        engine.enablePresentation(800, 600, "Vulkan Simple App");
        engine.init();
        engine.global.initiateShader_Triangle();
        engine.prepareDrawing();
        while (!glfwWindowShouldClose(engine.window)) {
            glfwPollEvents();
            engine.drawFrame();
        }
        //engine.shutdown();
    }
    Log("SimpleApp ended" << endl);
}
