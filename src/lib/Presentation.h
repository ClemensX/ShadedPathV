#pragma once

struct WindowInfo {
    int width = 800;
    int height = 600;
    const char* title = "Shaded Path Engine";
    GLFWwindow* glfw_window = nullptr;
	VkSurfaceKHR surface = nullptr;
	VkSwapchainKHR swapChain = nullptr;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat{};
	VkExtent2D swapChainExtent{};
	std::vector<VkImageView> swapChainImageViews;
	uint32_t imageCount;
	VkQueue presentQueue = nullptr;
	// copy backbuffer to swap chain image
	VkCommandBuffer commandBufferPresentBack;
	VkCommandBuffer commandBufferUI;
	VkSemaphore imageAvailableSemaphore = nullptr;
	VkSemaphore renderFinishedSemaphore = nullptr;
	VkSemaphore prePresentCompleteSemaphore = nullptr;
	VkFence inFlightFence = nullptr;
	VkEvent uiRenderFinished = nullptr;
	VkFence imageDumpFence = nullptr;
	VkFence presentFence = nullptr;
	// Debugging
	std::string commandBufferDebugName;
	bool disabled = false;
};

// for multi-app chaining
struct ContinuationInfo
{
	bool cont = false;
	WindowInfo windowInfo;
};

class Presentation : public EngineParticipant
{
public:
	Presentation(ShadedPathEngine* s);
	~Presentation();

	void initializeCallbacks(GLFWwindow* window, bool handleKeyEvents, bool handleMouseMoveEvents, bool handleMouseButtonEvents);
	void createWindow(WindowInfo* winfo, int w, int h, const char* name,
		bool handleKeyEvents = true, bool handleMouseMoveEevents = true, bool handleMouseButtonEvents = true);
	void createPresentQueue(unsigned int value);

    // only after win creation we can initiate Dear ImGui
	void startUI();
	void destroyWindowResources(WindowInfo* wi, bool destroyGlfwWindow = true);
    void detachFromWindow(WindowInfo* wi, ContinuationInfo* contInfo);
    void reuseWindow(WindowInfo* wi, ContinuationInfo* contInfo, bool handleKeyEvents = true, bool handleMouseMoveEevents = true, bool handleMouseButtonEvents = true);

	// poll events from glfw.
    // should be called from main thread and should work for all open glfw windows
	void pollEvents();

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
	void presentImage(FrameResources* fr, WindowInfo *winfo);
	void preparePresentation(WindowInfo* winfo);
	void endPresentation(WindowInfo* winfo);
	static void setKeyCallback(std::function<void(GLFWwindow*, int, int, int, int)> callback);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	void callbackKey(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void setMouseButtonCallback(std::function<void(GLFWwindow*, int, int, int)> callback);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	void callbackMouseButton(GLFWwindow* window, int button, int action, int mods);
	static void setCursorPosCallback(std::function<void(GLFWwindow* window, double x, double y)> callback);
	static void cursorPosCallback(GLFWwindow* window, double x, double y);
	void callbackCursorPos(GLFWwindow* window, double x, double y);
    GPUImage& getCreateGPUImage(uint32_t index, WindowInfo* winfo);
private:
	static std::function<void(GLFWwindow*, int, int, int, int)> currentKeyCallback;
	static std::function<void(GLFWwindow* window, int button, int action, int mods)> currentMouseButtonCallback;
	static std::function<void(GLFWwindow* window, double x, double y)> currentCursorPosCallback;
	bool glfwInitCalled = false;
	// event callbacks, will be called from main thread (via Presentation::pollEvents):

	static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	// Input will be handled via one instance - application code needs to copy if needed, not referenced
	InputState inputState;
	void createSwapChain(WindowInfo* winfo);
    std::vector<GPUImage> gpuImages; // store GPU images for presentation
};

