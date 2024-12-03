#include "mainheader.h"

using namespace std;

void ShadedPathEngine::initGlobal() {
    Log("initGlobal\n");
    globalRendering.initBeforePresentation();
    globalRendering.initAfterPresentation();
}

ShadedPathEngine::~ShadedPathEngine()
{
    Log("Engine destructor\n");
    for (auto& img : images) {
        destroyImage(&img);
    }
}

VkExtent2D ShadedPathEngine::getBackBufferExtent()
{
    return backBufferExtent;
}

void ShadedPathEngine::setBackBufferResolution(VkExtent2D e)
{
    if (initialized) Error("Configuration after intialization not allowed");
    backBufferExtent = e;
    backBufferAspect = (float)e.width / (float)e.height;
}

VkExtent2D ShadedPathEngine::getExtentForResolution(ShadedPathEngine::Resolution res)
{
    switch (res) {
    case Resolution::FourK:
        return { 3840, 2160 };
    case Resolution::TwoK:
        return { 1920, 1080 };
    case Resolution::OneK:
    case Resolution::DeviceDefault:
        return { 960, 540 };
    case Resolution::Small:
        return { 480, 270 };
    case Resolution::Invalid:
        return { 0, 0 };
    default:
        return { 960, 540 };
    }
}

void ShadedPathEngine::setBackBufferResolution(ShadedPathEngine::Resolution res)
{
    //if (shouldCloseApp) return;
    if (initialized) Error("Configuration after intialization not allowed");
    setBackBufferResolution(getExtentForResolution(res));
    //checkAspect();
}

GPUImage* ShadedPathEngine::createImage()
{
    GPUImage gpui;
    globalRendering.createImage(getBackBufferExtent().width, getBackBufferExtent().height, 1, VK_SAMPLE_COUNT_1_BIT, globalRendering.ImageFormat, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, gpui.image, gpui.memory);
    gpui.view = globalRendering.createImageView(gpui.image, globalRendering.ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    images.push_back(gpui);
    return &images.back();
}

void ShadedPathEngine::destroyImage(GPUImage* image)
{
    globalRendering.destroyImageView(image->view);
    globalRendering.destroyImage(image->image, image->memory);
}
