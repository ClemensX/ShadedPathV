#pragma once

//forward
struct WindowInfo;

struct InputState {
	glm::vec2 pos = glm::vec2(0.0f);
	// click event
	bool pressedLeft = false;
	bool pressedRight = false;
	// mouse button still pressed
	bool stillPressedLeft = false;
	bool stillPressedRight = false;

	int key = 0, scancode = 0, action = 0, mods = 0;
	bool keyEvent = false, mouseMoveEvent = false, mouseButtonEvent = false;
	WindowInfo* windowClosed = nullptr;
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

// low leve graphics: define image on GPU, that can be used as source or target
struct GPUImage {
	VkImage image = nullptr;
	VkDeviceMemory memory = nullptr;
	VkImageView view = nullptr;
    VkImageUsageFlags usage = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags access = 0;
	char* imagedata = nullptr;
	VkSubresourceLayout subResourceLayout;
    uint32_t width = 0, height = 0;
    bool rendered = false;
    bool consumed = false;
};

struct FrameInfo {
	long frameNum = -1;
//	long frameIndex = -1;
//	std::vector<VkSubmitInfo> submitinfos;
//	std::vector<VkCommandBuffer> activeCommandBuffers;
//	VkFence presentFence = nullptr;
//	ThreadsafeWaitingQueue<unsigned long> renderThreadContinueQueue;
//	VkPipelineStageFlags waitStages[2];
//#define	THREAD_RESOURCES_MAX_COMMAND_BUFFERS 10
//	VkCommandBuffer commandBuffers[THREAD_RESOURCES_MAX_COMMAND_BUFFERS];
//
//	// depth buffer
//	VkImage depthImage;
//	VkDeviceMemory depthImageMemory;
//	VkImageView depthImageView;
//	// right side views:
//	VkImage depthImage2;
//	VkDeviceMemory depthImageMemory2;
//	VkImageView depthImageView2;
//
//	bool threadFinished = false;

	// VR
#   if defined(OPENXR_AVAILABLE)	
	  //XrFrameState frameState{ XR_TYPE_FRAME_STATE };
	  //std::vector<XrCompositionLayerBaseHeader*> layers;
#   endif

	// Debugging
	std::string commandBufferDebugName;
	int threadResourcesIndex;
	GPUImage* renderedImage = nullptr;
};
