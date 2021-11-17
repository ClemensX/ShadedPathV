#include "pch.h"

void Shaders::initiateShader_Triangle()
{
}

bool Shaders::shouldClose()
{
	return false;
}

void Shaders::drawFrame_Triangle()
{
	// select the right thread resources
	auto& tr = engine.threadResources[engine.currentFrameIndex];
	//Log("draw index " << engine.currentFrameIndex << endl);

	// wait for fence signal
	//vkWaitForFences(engine.global.device, 1, &tr.inFlightFence, VK_TRUE, UINT64_MAX);
	//vkResetFences(engine.global.device, 1, &tr.inFlightFence);
}
