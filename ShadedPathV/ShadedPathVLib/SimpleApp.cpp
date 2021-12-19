#include "pch.h"


void SimpleApp::run()
{
    Log("SimpleApp started" << endl);
    {
        // engine configuration
        ShadedPathEngine engine;
        engine.gameTime.init(GameTime::GAMEDAY_1_MINUTE);
        //engine.setFrameCountLimit(1000);
        engine.setBackBufferResolution(ShadedPathEngine::Resolution::Small);
        engine.enablePresentation(800, (int)(800/1.77f), "Vulkan Simple App");
        engine.setFramesInFlight(20);
        //engine.setThreadModeSingle();

        // engine initialization
        engine.init();

        // shader initialization
        engine.shaders.initiateShader_Triangle();
        //engine.shaders.initiateShader_BackBufferImageDump();

        // some shaders may need additional preparation
        engine.prepareDrawing();

        // rendering
        while (!engine.shouldClose()) {
            engine.pollEvents();
            engine.drawFrame();
        }
        engine.waitUntilShutdown();
    }
    Log("SimpleApp ended" << endl);
}
