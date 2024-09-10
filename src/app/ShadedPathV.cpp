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
    //SimpleApp app; // ok
    //LineApp app; // ok
    //DeviceCoordApp app; // ok
    //gltfObjectsApp app; // ok
    gltfTerrainApp app; // ok
    //GeneratedTexturesApp app; // ok
    //BillboardDemo app; // ok
    //LandscapeDemo app; // ok
    //LandscapeGenerator app; // ok
    app.run();
}
