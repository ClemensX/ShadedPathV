#include "pch.h"

ShadedPathEngine* ThreadResources::engine = nullptr;

void ThreadResources::initAll(ShadedPathEngine* engine)
{
	ThreadResources::engine = engine;
	for (ThreadResources &res : engine->threadResources) {
		res.init();
	}
}

ThreadResources::ThreadResources()
{
	Log("new ThreadResource: " << this << endl);
}
void ThreadResources::init()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(engine->device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS) {
		Error("failed to create imageAvailableSemaphore for a frame");
	}
	if (vkCreateSemaphore(engine->device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
		Error("failed to create renderFinishedSemaphore for a frame");
	}

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // otherwise first wait() will wait forever

	if (vkCreateFence(engine->device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS) {
		Error("failed to create inFlightFence for a frame");
	}
}

ThreadResources::~ThreadResources()
{
	vkDestroySemaphore(engine->device, imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(engine->device, renderFinishedSemaphore, nullptr);
	vkDestroyFence(engine->device, inFlightFence, nullptr);
};

