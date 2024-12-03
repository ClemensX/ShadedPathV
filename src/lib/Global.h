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

// low leve graphics: define image on GPU, that can be used as source or target
struct GPUImage {
	VkImage image = nullptr;
	VkDeviceMemory memory = nullptr;
	VkImageView view = nullptr;
};
