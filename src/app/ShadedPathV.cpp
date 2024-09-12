// ShadedPathV.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "mainheader.h"
#include "AppSupport.h"
#include "LandscapeGenerator.h"
#include "SimpleApp.h"
#include "DeviceCoordApp.h"
#include "LineApp.h"
#include "gltfObjectsApp.h"
#include "gltfTerrainApp.h"
#include "GeneratedTexturesApp.h"
#include "BillboardDemo.h"
#include "LandscapeDemo1.h"

int main()
{
    Log("ShadedPathV app\n");
    //gltfTerrainApp app; // vr ok
    //LineApp app; // vr ok
    //SimpleApp app; // vr ok (some stuttering - will not be investigated)
    //DeviceCoordApp app; // vr not supported
    //BillboardDemo app; // vr ok
    //GeneratedTexturesApp app; // TODO: does not work even in 2D
    gltfObjectsApp app; // vr ok (skybox needs fixing: camera moves out of box)
    //LandscapeDemo app; //
    //LandscapeGenerator app; //
    app.run();
}
