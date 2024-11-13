#pragma once

// forward declarations
class ShadedPathEngine;

// all resources needed for running in a separate thread.
// it is ok to also READ from GlobalRendering and engine
class ThreadResources : public ShadedPathEngineParticipant
{
public:
	ThreadResources(ShadedPathEngine* engine_);
	virtual ~ThreadResources();

	// prevent copy and assigment
	ThreadResources(ThreadResources const&) = delete;
	void operator=(ThreadResources const&) = delete;
	// we need ability to move for putting into vector
	ThreadResources(ThreadResources&&) = default;
	ThreadResources& operator=(ThreadResources&&) = default;

	VkSemaphore imageAvailableSemaphore = nullptr;
	VkSemaphore renderFinishedSemaphore = nullptr;
	VkFence inFlightFence = nullptr;
	VkEvent uiRenderFinished = nullptr;
	void init();
	static void initAll(ShadedPathEngine* engine);

	VkCommandPool commandPool = nullptr;

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

	// signal switch to new set of resources
	unsigned long global_update_num = 0; // TODO cleanup - not used
	// store for each drawing thred which update set is currently in use
	GlobalUpdateElement* currentGlobalUpdateElement = nullptr;

	// ShaderThreadResources
	//ClearThreadResources clearResources;
	//LineThreadResources lineResources;
	PBRThreadResources pbrResources;
	SimpleThreadResources simpleResources;
	CubeThreadResources cubeResources;
	BillboardThreadResources billboardResources;
	// for ImGui:
	UIThreadResources uiResources;


	// frame management
	long frameNum = -1;
	long frameIndex = -1;
	std::vector<VkSubmitInfo> submitinfos;
	std::vector<VkCommandBuffer> activeCommandBuffers;
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
#   if defined(OPENXR_AVAILABLE)	
	  //XrFrameState frameState{ XR_TYPE_FRAME_STATE };
	  //std::vector<XrCompositionLayerBaseHeader*> layers;
#   endif

	// Debugging
	std::string commandBufferDebugName;
	int threadResourcesIndex;

private:
	void createFencesAndSemaphores();
	void createBackBufferImage();
	void createCommandPool();
	void createDepthResources();
	// we need to know which ThreadRessources we have for sub shaders in arrays
};
