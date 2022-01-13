#pragma once

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
	void update();
	// render pre-build UI
	// update() and render() are mutually exclusive (mutex protection inside)
	void render(ThreadResources& tr);
	VkRenderPass imGuiRenderPass = nullptr;
	void enable() {
		enabled = true;
	}
	bool isEnabled() {
		return enabled;
	}
private:
	void beginFrame();
	void buildUI();
	void endFrame();
	atomic<bool> enabled = false;
	ShadedPathEngine* engine = nullptr;
	VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
	mutable mutex monitorMutex;
	atomic<bool> uiRenderAvailable = false;
};

