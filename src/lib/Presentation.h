#pragma once

struct WindowInfo {
    int width = 800;
    int height = 600;
    const char* title = "Shaded Path Engine";
    GLFWwindow* glfw_window = nullptr;
	VkSurfaceKHR surface = nullptr;
	VkSwapchainKHR swapChain{};
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat{};
	VkExtent2D swapChainExtent{};
	std::vector<VkImageView> swapChainImageViews;
	uint32_t imageCount;
};

class Presentation : public EngineParticipant
{
public:
	Presentation(ShadedPathEngine* s);
	~Presentation();

	void createWindow(WindowInfo* winfo, int w, int h, const char* name,
		bool handleKeyEvents = true, bool handleMouseMoveEevents = true, bool handleMouseButtonEvents = true);
	// poll events from glfw.
    // should be called from main thread and should work for all open glfw windows
	void pollEvents();

	void createSwapChain(WindowInfo* winfo);
	// choose swap chain format or list available formats
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats, bool listmode = false);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, bool listmode = false);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

	// check if presentation is in use.
	// if yes, glfw has been initialized.
    bool isActive() {
        return windowInfo != nullptr;
    }

	// current input/output window
	WindowInfo* windowInfo = nullptr;

private:
    bool glfwInitCalled = false;
	// event callbacks, will be called from main thread (via Presentation::pollEvents):

	void callbackKey(GLFWwindow* window, int key, int scancode, int action, int mods);
	void callbackMouseButton(GLFWwindow* window, int button, int action, int mods);
	void callbackCursorPos(GLFWwindow* window, double x, double y);
	static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	// Input will be handled via one instance - application code needs to copy if needed, not referenced
	InputState inputState;
};

