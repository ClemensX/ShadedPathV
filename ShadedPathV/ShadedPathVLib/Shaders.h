#pragma once

struct ShaderState
{
	enum class StateEnum {
		// first stage - needs clearing depth, stencil and frame
		CLEAR,
		// transition from one shader to next in chain of render calls
		CONNECT,
		// prepare for final image copy
		PRESENT
	};
	VkViewport viewport{};
	VkRect2D scissor{};
	VkPipelineViewportStateCreateInfo viewportState{};
	// advance state, see impl for details
	// null ShaderBase sets last state PRESENT
	void advance(ShadedPathEngine* engine, ShaderBase* shader);
	StateEnum getState() {
		return state;
	}
private:
	StateEnum state = StateEnum::CLEAR;
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

		// Initialize ShaderState and all shaders
		Config& init();

		void createCommandBuffers(ThreadResources& tr);

		void setEngine(ShadedPathEngine* s) {
			engine = s;
		}

		void checkShaderState() {
			if (shaderState.viewport.height == 0) {
				Error("ShaderState not initialized. Did you run Shaders::initActiveShaders()?");
			}
		}
	private:
		vector<ShaderBase*> shaderList;
		ShadedPathEngine* engine;
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

	// Initialize ShaderState and all added shaders
	Shaders& initActiveShaders() {
		config.init();
		return *this;
	}

	// go through added shaders and initilaize thread local command buffers
	void createCommandBuffers(ThreadResources& tr);

	void checkShaderState(ShadedPathEngine& engine);

	// general methods
	void queueSubmit(ThreadResources& tr);
	VkShaderModule createShaderModule(const vector<byte>& code);

	// SHADERS

	// Triangle shader (most simple beginner shader - just one triangle, no MVP necessary)
	// initiate before rendering first frame
	SimpleShader simpleShader;
	LineShader lineShader;
	// submit command buffers for current frame
	void submitFrame(ThreadResources& tr);

	// BackBufferImageDump shader: copy backbuffer to image file
	// initiate before rendering first frame
	void initiateShader_BackBufferImageDump();
	// write backbuffer image to file (during frame creation)
	void executeBufferImageDump(ThreadResources& tr);

private:
	Config config;

	void initiateShader_BackBufferImageDumpSingle(ThreadResources& res);
	ShadedPathEngine& engine;

	unsigned int imageCouter = 0;
	bool enabledImageDump = false;
};

