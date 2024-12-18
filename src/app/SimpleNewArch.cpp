#include "mainheader.h"
#include "SimpleNewArch.h"

void SimpleApp::prepareFrame(FrameInfo* fi) {
    Log("prepareFrame " << fi->frameNum << std::endl);
    if (fi->frameNum >= 10) {
        shouldStop = true;
    }
    lastFrameNum = fi->frameNum;
};

// drawFrame is called for each topic in parallel!! Beware!
void SimpleApp::drawFrame(FrameInfo* fi, int topic) {
    if (fi->frameNum == 4 && topic == 0) {
        openWindow("Window Frame 4");
    }
    if (fi->frameNum == 8 && topic == 0) {
        openAnotherWindow("Another Win 8");
    }
    if (topic == 0) {
        Log("drawFrame " << fi->frameNum << " topic " << topic << std::endl);
        directImage.rendered = false;
        engine->util.writeRawImageTestData(directImage, 0);
        directImage.rendered = true;
        fi->renderedImage = &directImage;
    }
};

void SimpleApp::run() {
    Log("TestApp started\n");
    Log(" run thread: ");
    engine->log_current_thread();
    di.setEngine(engine);
    engine->configureParallelAppDrawCalls(2);
    gpui = engine->createImage("Test Image");
    engine->globalRendering.createDumpImage(directImage);
    di.openForCPUWriteAccess(gpui, &directImage);

    engine->eventLoop();

    // cleanup
    di.closeCPUWriteAccess(gpui, &directImage);
    engine->globalRendering.destroyImage(&directImage);

};

bool SimpleApp::shouldClose() {
    return shouldStop;
}

void SimpleApp::openWindow(const char* title) {
    Log("openWindow " << title << std::endl);
    int win_width = 960;//480;// 960;//1800;// 800;//3700; // 2500
    engine->enablePresentation(&window1, win_width, (int)(win_width / 1.77f), title);

}

void SimpleApp::openAnotherWindow(const char* title) {
    Log("openWindow " << title << std::endl);
    int win_width = 480;
    engine->enablePresentation(&window2, win_width, (int)(win_width / 1.77f), title);

}
