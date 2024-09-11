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
    //LandscapeDemo app; //
    BillboardDemo app; //
    //gltfObjectsApp app; //
    //GeneratedTexturesApp app; //
    //LandscapeGenerator app; //
    app.run();
}
