#include "pch.h"


void SimpleApp::run()
{
    Log("SimpleApp started" << endl);
    {
        // engine configuration
        ShadedPathEngine engine;
        engine.gameTime.init(GameTime::GAMEDAY_REALTIME);
        //engine.setFrameCountLimit(1000);
        engine.setBackBufferResolution(ShadedPathEngine::Resolution::OneK);
        int win_width = 960;
        engine.enablePresentation(win_width, (int)(win_width /1.77f), "Vulkan Simple App");
        engine.setFramesInFlight(2);
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
