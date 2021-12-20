#pragma once

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

// forward declarations
class ShadedPathEngine;
class ThreadResources;

class Presentation
{
public:
	Presentation(ShadedPathEngine& s) : engine(s) {
		Log("Presentation c'tor\n");
	};
	~Presentation();
	
	void init();
	void initAfterDeviceCreation();
	void initGLFW();
	void createPresentQueue(unsigned int value);


	// poll events from glfw
	void pollEvents();

	bool shouldClose();

	void possiblyAddDeviceExtensions(vector<const char*> &ext);

	// copy image from backbuffer to presentation window
	// prepare command buffer for copy of back buffer to swap chain images
	void initBackBufferPresentation();
	void initBackBufferPresentationSingle(ThreadResources &res);
	// perform the copy and display the image in app window
	void presentBackBufferImage(ThreadResources& tr);

	// if false we run on headless mode
	bool enabled = false;

	VkSurfaceKHR surface = nullptr;
	GLFWwindow* window = nullptr;
	VkQueue presentQueue = nullptr;
	VkSwapchainKHR swapChain{};
	vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat{};
	VkExtent2D swapChainExtent{};
	vector<VkImageView> swapChainImageViews;
	uint32_t imageCount;
private:
	ShadedPathEngine& engine;
	const vector<const char*> deviceExtensions = {
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

};

