#include "pch.h"


void SimpleApp::run()
{
    Log("SimpleApp started" << endl);
    {
        ShadedPathEngine engine;
        engine.gameTime.init(GameTime::GAMEDAY_1_MINUTE);
        engine.enablePresentation(800, 600, "Vulkan Simple App");
        engine.setFramesInFlight(2);
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
