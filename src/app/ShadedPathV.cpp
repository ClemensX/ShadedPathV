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
#include "TextureViewer.h"
#include "LandscapeDemo1.h"
#include "incoming.h"

int main()
{
    Log("ShadedPathV app\n");
    //Incoming app;
    //gltfTerrainApp app; // vr ok
    //LineApp app; // vr ok
    //SimpleApp app; // vr ok (some stuttering - will not be investigated)
    //DeviceCoordApp app; // vr not supported
    //BillboardDemo app; // vr ok
    TextureViewer app; // vr ok
    //GeneratedTexturesApp app; // TODO: does not even work in 2D
    //gltfObjectsApp app; // vr ok, also skybox
    //LandscapeDemo app; // vr ok
    //LandscapeGenerator app; // vr ok with limited support
    app.run();
}
