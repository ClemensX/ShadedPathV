#pragma once
class UI
{
public:
	void init(ShadedPathEngine *engine);
	~UI();
	VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;
	VkRenderPass imGuiRenderPass = nullptr;
};

