#pragma once

//forward
struct WindowInfo;
class ShadedPathEngine;

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

struct FrameBufferAttachment {
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
};

// low level graphics: define image on GPU, that can be used as source or target
struct GPUImage {
	VkImage image = nullptr;
	VkDeviceMemory memory = nullptr;
	VkImageView view = nullptr;
    VkImageUsageFlags usage = 0;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkAccessFlags2 access = 0;
    VkPipelineStageFlags2 stage = 0;
	char* imagedata = nullptr;
	VkSubresourceLayout subResourceLayout;
    uint32_t width = 0, height = 0;
    bool rendered = false;
    bool consumed = false;
};

// we either have command buffers or a rendered image as draw result
struct DrawResult {
    static const int MAX_COMMAND_BUFFERS_PER_DRAW = 100; // arbitrary, should be enough for most cases
    GPUImage* image = nullptr;
	// possibly many command buffers for each draw call / topic
    // the first nullptr indicates the end of the list
	std::array<VkCommandBuffer, MAX_COMMAND_BUFFERS_PER_DRAW> commandBuffers;
};

// FrameResources and ThreadResources work together: FrameResources has all all global frame data
// that is not related to multi-trhread rendering. ThreadResources has all thread specific data.
// FrameResources[2] --- engine.frameInfos[2], used alternating depending on frame number
//    - ThreadResources[engine.numWorkerThreads] --- globalRendering.workerThreadResources[], assigned to frameInfo in preFrame()
struct FrameResources {
    static void initAll(ShadedPathEngine* engine);
    void init();
    ShadedPathEngine* engine = nullptr;
    long frameNum = -1;
    // index into shader specific resources array (TODO: should be fixed resource element instead of index)
    long frameIndex = -1;
//	std::vector<VkSubmitInfo> submitinfos;
//	std::vector<VkCommandBuffer> activeCommandBuffers;
	VkFence presentFence = nullptr;
//	ThreadsafeWaitingQueue<unsigned long> renderThreadContinueQueue;
//	VkPipelineStageFlags waitStages[2];
//#define	THREAD_RESOURCES_MAX_COMMAND_BUFFERS 10
//	VkCommandBuffer commandBuffers[THREAD_RESOURCES_MAX_COMMAND_BUFFERS];
//
    VkSemaphore imageAvailableSemaphore = nullptr;
    VkSemaphore renderFinishedSemaphore = nullptr;
    VkFence inFlightFence = nullptr;
    VkEvent uiRenderFinished = nullptr;
    // depth buffer
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	// right side views:
	VkImage depthImage2;
	VkDeviceMemory depthImageMemory2;
	VkImageView depthImageView2;

    FrameBufferAttachment colorAttachment/*, depthAttachment*/;
    FrameBufferAttachment colorAttachment2; // right side view
    VkCommandPool commandPool = nullptr;

//
//	bool threadFinished = false;

	// VR
#   if defined(OPENXR_AVAILABLE)	
	  //XrFrameState frameState{ XR_TYPE_FRAME_STATE };
	  //std::vector<XrCompositionLayerBaseHeader*> layers;
#   endif

	// Debugging
	std::string commandBufferDebugName;
    GPUImage* renderedImage = nullptr;
    bool drawFrameDone = false;
    std::vector<DrawResult> drawResults; // one result for each draw call topic
    int numCommandBuffers = 0;

    // rest of this struct is for having a nice iterator.
    // use like this:
    // for (auto& commandBuffer : frameInfo) {
    //     Process each commandBuffer
    // }
    int countCommandBuffers() {
        int count = 0;
        forEachCommandBuffer([&count](VkCommandBuffer) {
            count++;
            });
        return count;
    }

    template <typename Func>
    void forEachCommandBuffer(Func func) {
        for (auto& drawResult : drawResults) {
            for (auto& commandBuffer : drawResult.commandBuffers) {
                if (commandBuffer == nullptr) {
                    break;
                }
                func(commandBuffer);
            }
        }
    }
private:
    void createFencesAndSemaphores();
    void createBackBufferImage();
    void createCommandPool();
    void createDepthResources();
};

class QueueSubmitResources
{
public:
	FrameResources* frameInfo = nullptr;
	ThreadsafeWaitingQueue<unsigned long> renderThreadContinueQueue;
};


class ThreadResources : public EngineParticipant {
public:
    virtual ~ThreadResources();
    FrameResources* frameInfo = nullptr;
    VkCommandPool commandPool = nullptr;
    VkCommandBuffer commandBuffer = nullptr;
};

// change image on GPU
class ImageWriter : public EngineParticipant
{
public:
    virtual ~ImageWriter() = 0;
    virtual void writeToImage(GPUImage* gpui) = 0;
};

// consume image on GPU (copy to CPU or to another GPU image)
class ImageConsumer : public EngineParticipant
{
public:
    virtual void consume(FrameResources* fi) = 0;
};
