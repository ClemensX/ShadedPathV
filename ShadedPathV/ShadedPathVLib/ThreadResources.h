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

	VkCommandPool commandPool = nullptr;
	VkFramebuffer framebuffer = nullptr;
	// ImGui framebuffers
	VkFramebuffer framebufferUI = nullptr;

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

	// global resources shared by all shaders
	FrameBufferAttachment colorAttachment, depthAttachment;

	// triangle shader / SimpleShader
	VkPipelineLayout pipelineLayoutTriangle = nullptr;
	VkPipeline graphicsPipelineTriangle = nullptr;
	VkRenderPass renderPassSimpleShader = nullptr;
	VkCommandBuffer commandBufferTriangle = nullptr;
	VkBuffer uniformBufferTriangle;
	VkDeviceMemory uniformBufferMemoryTriangle;
	VkDescriptorSet descriptorSetTriangle = nullptr;

	// Line shader
	VkFramebuffer framebufferLine = nullptr;
	VkPipelineLayout pipelineLayoutLine = nullptr;
	VkPipeline graphicsPipelineLine = nullptr;
	VkRenderPass renderPassLine = nullptr;
	VkCommandBuffer commandBufferLine = nullptr;
	VkCommandBuffer commandBufferLineAdd = nullptr;
	VkBuffer vertexBufferAdd;
	VkBuffer uniformBufferLine;
	VkDeviceMemory uniformBufferMemoryLine;
	VkDescriptorSet descriptorSetLine = nullptr;
	LineFrameData lineFrameData;

	// frame management
	long frameNum = -1;
	long frameIndex = -1;
	vector<VkSubmitInfo> submitinfos;
	VkFence presentFence = nullptr;
	ThreadsafeWaitingQueue<unsigned long> renderThreadContinueQueue;
	VkPipelineStageFlags waitStages[2];
	VkCommandBuffer commandBuffers[5];

	// depth buffer
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	bool threadFinished = false;
private:
	void createFencesAndSemaphores();
	void createFrameBufferUI();
	void createBackBufferImage();
	void createCommandPool();
	void createDepthResources();
};
