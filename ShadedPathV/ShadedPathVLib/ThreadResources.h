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
	VkFence inFlightFence = nullptr;
	void init();
	static void initAll(ShadedPathEngine* engine);

	VkPipelineLayout pipelineLayoutTriangle = nullptr;
	VkPipeline graphicsPipelineTriangle = nullptr;
	VkRenderPass renderPass = nullptr;
	VkCommandPool commandPool;
	VkFramebuffer framebuffer = nullptr;

	// for each shader: command buffer 
	void createCommandBufferTriangle();
	VkCommandBuffer commandBufferTriangle;
private:
	void createFencesAndSemaphores();
	void createRenderPass();
	void createFrameBuffer();
	void createImage();
	void createCommandPool();
	FrameBufferAttachment colorAttachment, depthAttachment;
};

