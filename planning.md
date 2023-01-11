# Planning

## Vulkan Resources Use

Current (Q1 2023) use of Vulkan resources per shader:

| **SimpleShader** |   |
| ---      | ---           |
| GLSL Shaders  | vert, frag    | 
| **Single Resources:**
| VkShaderModule   | vert, frag | 
| VkBuffer   | vertexBufferTriangle, indexBufferTriangle | 
| **Threaded Resources:**
| VkBuffer  | UBO for MVP: uniformBuffer | 
| VkRenderPass  | renderPass | 
| VkFrameBuffer  | frameBuffer | 
| VkFrameBuffer  | frameBuffer2 (for stereo) | 
| VkPipeline | graphicsPipeline
| **Descriptor Set Bindings:** (per render thread)
| Binding 0 | VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER VK_SHADER_STAGE_VERTEX_BIT
| Binding 1 | VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER VK_SHADER_STAGE_FRAGMENT_BIT
| **Descriptor Pools:** |
| poolSizeCount | 4
| maxSets | 5