#pragma once

class Shaders : public EngineParticipant
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

		void createCommandBuffers(FrameResources& tr);
		void gatherActiveCommandBuffers(FrameResources& tr);

		// destroy shader thread resources
		void destroyThreadResources(FrameResources& tr);

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
	Shaders(ShadedPathEngine* s) {
		Log("Shaders c'tor\n");
		setEngine(s);
		config.setEngine(s);
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
		auto& shaders = getShaders();
		if (shaders.size() == 0) Error("No shaders added to Shaders object");
		auto shaderInstance = shaders.back();
		// check subclass for EndShader
		EndShader* derivedPtr = dynamic_cast<EndShader*>(shaderInstance);
		if (derivedPtr == nullptr) {
			// last shader should be EndShader
			config.add(endShader);
		}
		config.init();
		return *this;
	}

	// go through added shaders and initilaize thread local command buffers
	void createCommandBuffers(FrameResources& tr);

	void gatherActiveCommandBuffers(FrameResources& tr);
	void checkShaderState(ShadedPathEngine& engine);

	// destroy all shader thread local resources
	void destroyThreadResources(FrameResources& tr);

	// general methods
	void queueSubmit(FrameResources& tr);
	VkShaderModule createShaderModule(const std::vector<std::byte>& code);

	// SHADERS. All shaders instances are here, but each shader has to be activated in application code

	ClearShader clearShader;
	EndShader endShader;
	//SimpleShader simpleShader;
	LineShader lineShader;
	//UIShader uiShader;
	//PBRShader pbrShader;
	//CubeShader cubeShader;
	//BillboardShader billboardShader;
	//TerrainShader terrainShader;

	// submit command buffers for current frame
	void submitFrame(ThreadResources& tr);

	// BackBufferImageDump: copy backbuffer to image file
	// initiate before rendering first frame
	// either all frames will be dumped (mainly for automated tests)
	// or a single frame after calling backBufferImageDumpNextFrame()
	// engine should be in single thread mode for image dumps
	void initiateShader_BackBufferImageDump(bool dumpAll = true); // default: dump all frames
	// write backbuffer image to file (during frame creation)
	void executeBufferImageDump(ThreadResources& tr);
	// dump next frame only
	void backBufferImageDumpNextFrame() {
		enabledImageDumpForNextFrame = true;
	}

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

	unsigned int imageCounter = 0;
	bool enabledAllImageDump = false;
	bool enabledImageDumpForNextFrame = false;
};

