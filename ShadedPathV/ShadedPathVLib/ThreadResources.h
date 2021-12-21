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

	// prevent copy and assigment
	ThreadResources(ThreadResources const&) = delete;
	void operator=(ThreadResources const&) = delete;
	// we need ability to move for putting into vector
	ThreadResources(ThreadResources&&) = default;
	ThreadResources& operator=(ThreadResources&&) = default;

	static ShadedPathEngine* engine;
	VkSemaphore imageAvailableSemaphore = nullptr;
	VkSemaphore renderFinishedSemaphore = nullptr;
	VkFence inFlightFence = nullptr;
	void init();
	static void initAll(ShadedPathEngine* engine);

	VkPipelineLayout pipelineLayoutTriangle = nullptr;
	VkPipeline graphicsPipelineTriangle = nullptr;
	VkRenderPass renderPass = nullptr;
	VkCommandPool commandPool = nullptr;
	VkFramebuffer framebuffer = nullptr;

	// backbuffer image dump:
	const char* imagedata = nullptr;
	FrameBufferAttachment imageDumpAttachment{};
	VkCommandBuffer commandBufferImageDump;
	VkFence imageDumpFence = nullptr;
	VkSubresourceLayout subResourceLayout;

	// copy backbuffer to swap chain image
	VkCommandBuffer commandBufferPresentBack;

	// triangle shader 
	void createCommandBufferTriangle();
	FrameBufferAttachment colorAttachment, depthAttachment;
	VkCommandBuffer commandBufferTriangle;

	// frame management
	long frameNum = -1;
	long frameIndex = -1;
	vector<VkSubmitInfo> submitinfos;
	VkFence presentFence = nullptr;
	//atomic_flag* renderThreadContinue = nullptr;
	//ThreadsafeWaitingQueue<unsigned long> renderThreadContinueQueue(unsigned long, true, "renderThreadContinue");
	//typedef ThreadsafeWaitingQueue<unsigned long> RenderThreadContinueQueue;
	//RenderThreadContinueQueue renderThreadContinueQueue();
	//RenderThreadContinueQueue q(0, const false, "");
	ThreadsafeWaitingQueue<unsigned long> renderThreadContinueQueue;


	bool threadFinished = false;
private:
	void createFencesAndSemaphores();
	void createRenderPass();
	void createFrameBuffer();
	void createBackBufferImage();
	void createCommandPool();
};
