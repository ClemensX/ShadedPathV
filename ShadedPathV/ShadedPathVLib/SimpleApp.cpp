#include "pch.h"


void SimpleApp::run()
{
    Log("SimpleApp started" << endl);
    {
        ShadedPathEngine engine;
        engine.setFrameCountLimit(4);
        engine.gameTime.init(GameTime::GAMEDAY_1_MINUTE);
        engine.enablePresentation(800, 600, "Vulkan Simple App");
        engine.setFramesInFlight(2);
        engine.init();
        engine.shaders.initiateShader_Triangle();
        engine.shaders.initiateShader_BackBufferImageDump();
        engine.prepareDrawing();
        while (!engine.shouldClose()) {
            engine.pollEvents();
            engine.drawFrame();
        }
    }
    Log("SimpleApp ended" << endl);
}
