// ShadedPathV.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "mainheader.h"
#include "AppSupport.h"
#include "SimpleMultiApp.h"
#include "SimpleMultiWin.h"
#include "LineApp.h"
#include "SimpleApp.h"
#include "DeviceCoordApp.h"
#include "GeneratedTexturesApp.h"
#include "gltfTerrainApp.h"
#include "gltfObjectsApp.h"
#include "BillboardDemo.h"
#include "TextureViewer.h"
#include "LandscapeDemo1.h"
#include "incoming.h"
#include "LandscapeGenerator.h"
#include "Loader.h"

int main()
{
    //mainSimpleMultiWin(); return 0; // use with care, see notes in SimpleMultiWin.h
    //mainSimpleMultiApp(); return 0;

    //LineApp app; // vr ok
    //SimpleApp app; // vr ok (some stuttering - will not be investigated)
    //DeviceCoordApp app; // vr not supported
    //gltfTerrainApp app; // vr ok
    //GeneratedTexturesApp app; // TODO: does not even work in 2D
    //gltfObjectsApp app; // vr ok, also skybox
    //BillboardDemo app; // vr ok
    //TextureViewer app; // vr ok
    //LandscapeDemo app; // vr ok
    //Incoming app;
    //LandscapeGenerator app; // vr ok with limited support
    Loader app;

    Log("main() start!\n");
    ShadedPathEngine engine;
    engine
        .setEnableLines(true)
        .setDebugWindowPosition(true)
        .setEnableUI(true)
        .setEnableSound(true)
        .setVR(false)
        //.setStereo(true)
        .failIfNoVR(true)
        //.setSingleThreadMode(true)
        //.enableStereoPresentation()
        .overrideCPUCores(4)
        .configureParallelAppDrawCalls(2)
        .setMaxTextures(50)
        //.setFixedPhysicalDeviceIndex(1) // if GPU card is available it should be device 0. device 1 is usually the integrated Intel/AMD GPU
        ;

    engine.initGlobal();
    ContinuationInfo cont;
    engine.registerApp((ShadedPathApplication*)&app);
    engine.app->run(&cont);
}

