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
    //LandscapeDemo app; // vr ok
    //LineApp app; // vr ok
    SimpleApp app; //
    //DeviceCoordApp app; // vr not supported
    //gltfObjectsApp app; //
    //gltfTerrainApp app; //
    //GeneratedTexturesApp app; //
    //BillboardDemo app; //
    //LandscapeGenerator app; //
    app.run();
}
