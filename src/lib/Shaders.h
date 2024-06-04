#pragma once

// ShaderState will be used during initial shader setup only (not for regular frame rendering).
// it tracks state that has to be accessed by more than one shader
struct ShaderState
{
	VkViewport viewport{};
	VkRect2D scissor{};
	VkPipelineViewportStateCreateInfo viewportState{};

	// various flags, usually set by shaders init() or initSingle()

	// signal first shader that should clear depth and framebuffers
	bool isClear = false;
	bool isPresent = false;
};

class Shaders
{
	class Config {
	public:
		// add next shader to list of shaders
		// shaders will be called in the order they were added here.
		Config& add(ShaderBase& shader) {
			shaderList.push_back(&shader);
			return *this;
		}

		std::vector<ShaderBase*>& getShaders() {
			return shaderList;
		}

		// Initialize ShaderState and all shaders
		Config& init();

		void doGlobalUpdate();
		void createCommandBuffers(ThreadResources& tr);
		void gatherActiveCommandBuffers(ThreadResources& tr);

		// destroy shader thread resources
		void destroyThreadResources(ThreadResources& tr);

		void setEngine(ShadedPathEngine* s) {
			engine = s;
		}

		void checkShaderState() {
			if (shaderState.viewport.height == 0) {
				Error("ShaderState not initialized. Did you run Shaders::initActiveShaders()?");
			}
		}
	private:
		std::vector<ShaderBase*> shaderList;
		ShadedPathEngine* engine = nullptr;
		ShaderState shaderState;
	};
public:
	Shaders(ShadedPathEngine& s) : engine(s) {
		Log("Shaders c'tor\n");
		config.setEngine(&s);
	};
	~Shaders();

	// add next shader to list of shaders
	// shaders will be called in the order they were added here.
	Shaders& addShader(ShaderBase& shader) {
		config.add(shader);
		return *this;
	}

	std::vector<ShaderBase*>& getShaders() {
		return config.getShaders();
	}

	// Initialize ShaderState and all added shaders
	Shaders& initActiveShaders() {
		config.init();
		return *this;
	}

	// go through added shaders and initilaize thread local command buffers
	void createCommandBuffers(ThreadResources& tr);

	void gatherActiveCommandBuffers(ThreadResources& tr);
	void checkShaderState(ShadedPathEngine& engine);

	// destroy all shader thread local resources
	void destroyThreadResources(ThreadResources& tr);

	// perform expensive updates in global update thread
	void doGlobalUpdate();

	// general methods
	void queueSubmit(ThreadResources& tr);
	VkShaderModule createShaderModule(const std::vector<std::byte>& code);

	// SHADERS. All shaders instances are here, but each shader has to be activated in application code

	ClearShader clearShader;
	SimpleShader simpleShader;
	LineShader lineShader;
	UIShader uiShader;
	PBRShader pbrShader;
	CubeShader cubeShader;
	BillboardShader billboardShader;

	// submit command buffers for current frame
	void submitFrame(ThreadResources& tr);

	// BackBufferImageDump shader: copy backbuffer to image file
	// initiate before rendering first frame
	void initiateShader_BackBufferImageDump();
	// write backbuffer image to file (during frame creation)
	void executeBufferImageDump(ThreadResources& tr);

	VkPipelineShaderStageCreateInfo createVertexShaderCreateInfo(VkShaderModule& shaderModule) {
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = shaderModule;
		vertShaderStageInfo.pName = "main";
		return vertShaderStageInfo;
	}

	VkPipelineShaderStageCreateInfo createMeshShaderCreateInfo(VkShaderModule& shaderModule) {
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_MESH_BIT_NV;
		fragShaderStageInfo.module = shaderModule;
		fragShaderStageInfo.pName = "main";
		return fragShaderStageInfo;
	}

	VkPipelineShaderStageCreateInfo createFragmentShaderCreateInfo(VkShaderModule& shaderModule) {
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = shaderModule;
		fragShaderStageInfo.pName = "main";
		return fragShaderStageInfo;
	}

	VkPipelineShaderStageCreateInfo createGeometryShaderCreateInfo(VkShaderModule& shaderModule) {
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		fragShaderStageInfo.module = shaderModule;
		fragShaderStageInfo.pName = "main";
		return fragShaderStageInfo;
	}

private:
	Config config;

	void initiateShader_BackBufferImageDumpSingle(ThreadResources& res);
	ShadedPathEngine& engine;

	unsigned int imageCouter = 0;
	bool enabledImageDump = false;
};

