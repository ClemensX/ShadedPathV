#pragma once

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
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

// forward declarations
class ShadedPathEngine;
class ThreadResources;

class Presentation : public EngineParticipant
{
public:
	Presentation(ShadedPathEngine* s) {
		Log("Presentation c'tor\n");
        setEngine(s);
	};
	~Presentation();
	
	void init();
	void initAfterDeviceCreation();
	void initGLFW(bool handleKeyEvents, bool handleMouseMoveEevents, bool handleMouseButtonEvents);
	void createPresentQueue(unsigned int value);


	// poll events from glfw
	void pollEvents();

	bool shouldClose();

	void possiblyAddDeviceExtensions(std::vector<const char*> &ext);

	// copy image from backbuffer to presentation window
	// prepare command buffer for copy of back buffer to swap chain images
	void initBackBufferPresentation();
	void initBackBufferPresentationSingle(ThreadResources &res);
	// Thread independent preparations, e.g. OpenXR beginFrame()
	void beginPresentFrame(ThreadResources& tr);
	// perform the copy and display the image in app window
	void presentBackBufferImage(ThreadResources& tr);

	// if false we run on headless mode
	bool enabled = false;

	VkSurfaceKHR surface = nullptr;
	GLFWwindow* window = nullptr;
	VkQueue presentQueue = nullptr;
	VkSwapchainKHR swapChain{};
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat{};
	VkExtent2D swapChainExtent{};
	std::vector<VkImageView> swapChainImageViews;
	uint32_t imageCount;
private:
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	void createSurface();
	void createSwapChain();
	void createImageViews();
	// choose swap chain format or list available formats
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, bool listmode = false);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool listmode = false);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	// event callbacks, will be called from main thread (via Presentation::pollEvents):

	void callbackKey(GLFWwindow* window, int key, int scancode, int action, int mods);
	void callbackMouseButton(GLFWwindow* window, int button, int action, int mods);
	void callbackCursorPos(GLFWwindow* window, double x, double y);
	static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	// Input will be handled via one instance - application code needs to copy if needed, not referenced
	InputState inputState;
	static uint32_t threadId;
};

