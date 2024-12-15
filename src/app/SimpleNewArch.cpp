#include "mainheader.h"
#include "SimpleNewArch.h"

void SimpleApp::prepareFrame(FrameInfo* fi) {
    Log("prepareFrame " << fi->frameNum << std::endl);
    if (fi->frameNum >= 10) {
        shouldStop = true;
    }
    lastFrameNum = fi->frameNum;
};

void SimpleApp::drawFrame(FrameInfo* fi) {
    Log("drawFrame " << fi->frameNum << std::endl);
    engine->util.writeRawImageTestData(directImage, 0);
};

void SimpleApp::run() {
    Log("TestApp started\n");
    Log(" run thread: ");
    engine->log_current_thread();
    di.setEngine(engine);
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
