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
	// ImGui framebuffers
	VkFramebuffer framebufferUI = nullptr;

	// backbuffer image dump:
	const char* imagedata = nullptr;
	FrameBufferAttachment imageDumpAttachment{};
	VkCommandBuffer commandBufferImageDump = nullptr;
	VkFence imageDumpFence = nullptr;
	VkSubresourceLayout subResourceLayout;

	// copy backbuffer to swap chain image
	VkCommandBuffer commandBufferPresentBack;

	// draw UI
	VkCommandBuffer commandBufferUI;

	// global resources shared by all shaders
	FrameBufferAttachment colorAttachment/*, depthAttachment*/;
	FrameBufferAttachment colorAttachment2; // right side view

	ClearThreadResources clearResources;
	// ClearShader
	//VkPipelineLayout pipelineLayoutTriangle = nullptr;
	//VkPipeline graphicsPipelineTriangle = nullptr;
	VkRenderPass renderPassClear = nullptr;
	VkCommandBuffer commandBufferClear = nullptr;
	//VkFramebuffer framebufferClear = nullptr;
	VkFramebuffer framebufferClear2 = nullptr;
	//VkBuffer uniformBufferTriangle;
	//VkDeviceMemory uniformBufferMemoryTriangle;
	//VkDescriptorSet descriptorSetTriangle = nullptr;

	// triangle shader / SimpleShader
	VkPipelineLayout pipelineLayoutTriangle = nullptr;
	VkPipeline graphicsPipelineTriangle = nullptr;
	VkRenderPass renderPassSimpleShader = nullptr;
	VkCommandBuffer commandBufferTriangle = nullptr;
	VkBuffer uniformBufferTriangle = nullptr;
	VkDeviceMemory uniformBufferMemoryTriangle = nullptr;
	VkDescriptorSet descriptorSetTriangle = nullptr;
	VkFramebuffer framebufferSimple = nullptr;
	VkFramebuffer framebufferSimple2 = nullptr;

	// Line shader
	VkFramebuffer framebufferLine = nullptr;
	VkFramebuffer framebufferLineAdd = nullptr;
	VkFramebuffer framebufferLine2 = nullptr;
	VkFramebuffer framebufferLineAdd2 = nullptr;
	VkRenderPass renderPassLine = nullptr;
	VkRenderPass renderPassLineAdd = nullptr;
	VkPipelineLayout pipelineLayoutLine = nullptr;
	VkPipeline graphicsPipelineLine = nullptr;
	VkPipeline graphicsPipelineLineAdd = nullptr;
	VkCommandBuffer commandBufferLine = nullptr;
	VkCommandBuffer commandBufferLineAdd = nullptr;
	// vertex buffer for added lines
	VkBuffer vertexBufferAdd = nullptr;
	// vertex buffer device memory
	VkDeviceMemory vertexBufferAddMemory = nullptr;
	// MVP buffer
	VkBuffer uniformBufferLine = nullptr;
	VkBuffer uniformBufferLine2 = nullptr;
	// MVP buffer device memory
	VkDeviceMemory uniformBufferMemoryLine = nullptr;
	VkDeviceMemory uniformBufferMemoryLine2 = nullptr;
	VkDescriptorSet descriptorSetLine = nullptr;
	VkDescriptorSet descriptorSetLine2 = nullptr;
	vector<LineShader::Vertex> verticesAddLines;

	// frame management
	long frameNum = -1;
	long frameIndex = -1;
	vector<VkSubmitInfo> submitinfos;
	vector<VkCommandBuffer> activeCommandBuffers;
	VkFence presentFence = nullptr;
	ThreadsafeWaitingQueue<unsigned long> renderThreadContinueQueue;
	VkPipelineStageFlags waitStages[2];
#define	THREAD_RESOURCES_MAX_COMMAND_BUFFERS 10
	VkCommandBuffer commandBuffers[THREAD_RESOURCES_MAX_COMMAND_BUFFERS];

	// depth buffer
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	// right side views:
	VkImage depthImage2;
	VkDeviceMemory depthImageMemory2;
	VkImageView depthImageView2;

	bool threadFinished = false;

	// VR
	XrFrameState frameState{ XR_TYPE_FRAME_STATE };
	vector<XrCompositionLayerBaseHeader*> layers;

private:
	void createFencesAndSemaphores();
	void createBackBufferImage();
	void createCommandPool();
	void createDepthResources();
};
