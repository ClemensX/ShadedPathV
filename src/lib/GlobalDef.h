// basic line definitions
struct LineDef {
	glm::vec3 start, end;
	glm::vec4 color;
};

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

static const int MAX_COMMAND_BUFFERS_PER_DRAW = 100; // arbitrary, should be enough for most cases
