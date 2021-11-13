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
		Error("failed to create semaphores for a frame");
	}
	if (vkCreateSemaphore(engine->device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
		Error("failed to create semaphores for a frame");
	}
}

ThreadResources::~ThreadResources()
{
	vkDestroySemaphore(engine->device, imageAvailableSemaphore, nullptr);
	vkDestroySemaphore(engine->device, renderFinishedSemaphore, nullptr);
};

