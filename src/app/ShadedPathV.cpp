// ShadedPathV.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "mainheader.h"
#include "AppSupport.h"
#include "SimpleMultiApp.h"
#include "SimpleMultiWin.h"
//#include "AppSupport.h"
//#include "LandscapeGenerator.h"
#include "SimpleApp.h"
//#include "DeviceCoordApp.h"
#include "LineApp.h"
//#include "gltfObjectsApp.h"
//#include "gltfTerrainApp.h"
//#include "GeneratedTexturesApp.h"
//#include "BillboardDemo.h"
//#include "TextureViewer.h"
//#include "LandscapeDemo1.h"
//#include "incoming.h"


int main()
{
    //mainSimpleMultiWin(); return 0; // use with care, see notes in SimpleMultiWin.h
    //mainSimpleMultiApp(); return 0;

    LineApp app;
    //SimpleApp app;

    Log("main() start!\n");
    ShadedPathEngine engine;
    engine
        .setEnableLines(true)
        .setDebugWindowPosition(true)
        .setEnableUI(true)
        .setEnableSound(true)
        .setVR(true)
        .setStereo(true)
        .failIfNoVR(true)
        //.setSingleThreadMode(true)
        .overrideCPUCores(4)
        .configureParallelAppDrawCalls(2)
        //.setFixedPhysicalDeviceIndex(1) // if GPU card is available it should be device 0. device 1 is usually the integrated Intel/AMD GPU
        ;

    engine.initGlobal();
    ContinuationInfo cont;
    engine.registerApp((ShadedPathApplication*)&app);
    engine.app->run(&cont);
}

