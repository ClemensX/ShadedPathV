#pragma once

// forward declarations
class ShadedPathEngine;

// all resources needed for running in a separate thread.
// it is ok to also READ from GlobalRendering and engine
class ThreadResources
{
public:
	ThreadResources();
	virtual ~ThreadResources();
	static ShadedPathEngine* engine;
	VkSemaphore imageAvailableSemaphore = nullptr;
	VkSemaphore renderFinishedSemaphore = nullptr;
	void init();
	static void initAll(ShadedPathEngine* engine);
};

