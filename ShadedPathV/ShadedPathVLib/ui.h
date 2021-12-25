#pragma once

// UI class abtraction for Dear ImGui
// Dear ImGui is strictly single threaded and all the methoods here have to be used from main thread
// because ImGui needs access to the glfw window and input cycle
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
private:
	void beginFrame();
	void buildUI();
	void endFrame();
	ShadedPathEngine* engine = nullptr;
	VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
	mutable mutex monitorMutex;
	atomic<bool> uiRenderAvailable = false;
};

