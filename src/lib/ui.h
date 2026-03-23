#pragma once

// forward
class UISubShader;

enum UIRenderFlags {
	UIRender_FPS = 0x01,
	UIRender_CameraPosDir = 0x02,
	UIRender_MouseTracking = 0x04,
};

// UI class abtraction for Dear ImGui
// Dear ImGui is strictly single threaded and all the methods here have to be used from queue submit thread
// because ImGui needs access to the glfw window and input cycle,
// because of that we can do with one single render pass, but still need framebuffers for every image

class UI
{
public:
	void init(ShadedPathEngine *engine);
	~UI();

	// build new UI for this frame
	// update() and render() are mutually exclusive (mutex protection inside)
	// only called from ShadedPathEngine::pollEvents()
	void update();
	// render pre-build UI
	// update() and render() are mutually exclusive (mutex protection inside)
	void render(FrameResources* fr, UISubShader* pf);
	void setRenderFlags(unsigned int flags) {
		renderFlags = flags;
	}
	bool hasRenderFlag(UIRenderFlags flag) const {
		return (renderFlags & flag) != 0;
	}
	VkRenderPass imGuiRenderPass = nullptr;
	void enable() {
		enabled = true;
	}
	bool isEnabled() {
		return enabled;
	}
    void setCameraForTracking(const Camera* cam) {
		cameraForTracking = cam;
	}
private:
	void beginFrame();
	void buildUI();
	void endFrame();
	std::atomic<bool> enabled = false;
	ShadedPathEngine* engine = nullptr;
	VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
	mutable std::mutex monitorMutex;
	// default render flags for FPS display only
    unsigned int renderFlags = UIRender_FPS;
	std::atomic<bool> uiRenderAvailable = false;
	const Camera* cameraForTracking = nullptr;
};

