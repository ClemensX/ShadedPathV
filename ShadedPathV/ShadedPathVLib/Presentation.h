#pragma once

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

// forward declarations
class ShadedPathEngine;

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


	// poll events from glfw
	void pollEvents();

	bool shouldClose();

	void possiblyAddDeviceExtensions(vector<const char*> &ext);

	// if false we run on headless mode
	bool enabled = false;

	VkSurfaceKHR surface = nullptr;
	GLFWwindow* window = nullptr;
private:
	ShadedPathEngine& engine;
	const vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	void createSurface();
	// choose swap chain format or list available formats
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, bool listmode = false);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool listmode = false);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

};
