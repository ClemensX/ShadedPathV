// ShadedPathV.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include "mainheader.h"
#include "LandscapeGenerator.h"
#include "SimpleApp.h"
#include "DeviceCoordApp.h"
#include "LineApp.h"
#include "gltfObjectsApp.h"
#include "GeneratedTexturesApp.h"
#include "BillboardDemo.h"
#include "LandscapeDemo1.h"

int main()
{
    Log("ShadedPathV app\n");
    //SimpleApp app;
    LineApp app;
    //DeviceCoordApp app;
    //gltfObjectsApp app;
    //GeneratedTexturesApp app;
    //BillboardDemo app;
    //LandscapeDemo app;
    //LandscapeGenerator app;
    app.run();
}
