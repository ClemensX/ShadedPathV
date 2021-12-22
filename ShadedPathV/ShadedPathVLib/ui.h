#pragma once

// UI class abtraction for Dear ImGui
// Dear ImGui is strictly single threaded and all the methoods here have to be used from main thread
// because ImGui needs access to the glfw window and input cycle
// TODO find a way to render UI in render threads or during presenting
class UI
{
public:
	void init(ShadedPathEngine *engine);
	~UI();

	// build new UI for this frame
	void update(ThreadResources& tr);
	// render pre-build UI
	void render(ThreadResources& tr);
private:
	void beginFrame(ThreadResources& tr);
	void buildUI(ThreadResources& tr);
	void endFrame(ThreadResources& tr);
	ShadedPathEngine* engine = nullptr;
	VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
	VkRenderPass imGuiRenderPass = nullptr;
};

