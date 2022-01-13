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
	VkEvent uiRenderFinished = nullptr;
	void init();
	static void initAll(ShadedPathEngine* engine);

	VkPipelineLayout pipelineLayoutTriangle = nullptr;
	VkPipeline graphicsPipelineTriangle = nullptr;
	VkRenderPass renderPassInit = nullptr;
	VkCommandPool commandPool = nullptr;
	VkFramebuffer framebuffer = nullptr;
	VkFramebuffer framebufferUI = nullptr;
	VkDescriptorPool descriptorPool = nullptr;

	// backbuffer image dump:
	const char* imagedata = nullptr;
	FrameBufferAttachment imageDumpAttachment{};
	VkCommandBuffer commandBufferImageDump;
	VkFence imageDumpFence = nullptr;
	VkSubresourceLayout subResourceLayout;

	// copy backbuffer to swap chain image
	VkCommandBuffer commandBufferPresentBack;

	// draw UI
	VkCommandBuffer commandBufferUI;

	// triangle shader / SimpleShader
	FrameBufferAttachment colorAttachment, depthAttachment;
	VkCommandBuffer commandBufferTriangle;
	VkBuffer uniformBufferTriangle;
	VkDeviceMemory uniformBufferMemoryTriangle;
	VkDescriptorSet descriptorSetTriangle = nullptr;

	// frame management
	long frameNum = -1;
	long frameIndex = -1;
	vector<VkSubmitInfo> submitinfos;
	VkFence presentFence = nullptr;
	ThreadsafeWaitingQueue<unsigned long> renderThreadContinueQueue;

	// depth buffer
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	bool threadFinished = false;
private:
	void createFencesAndSemaphores();
	void createRenderPassInit();
	void createRenderPassDraw();
	void createFrameBuffer();
	void createBackBufferImage();
	void createCommandPool();
	void createDepthResources();
	void createDescriptorPool();
};
