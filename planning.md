# Planning

## Vulkan Resources Use

Current (Q1 2023) use of Vulkan resources per shader:

SimpleShader is a playfield to try out rendering techniques:
a single texture is rednered in 2 rectangles that keep rotating

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

Clear Shader is not a complete shader, but is auto-added to the first shader in the list to clear color and depth attachement

| **ClearShader** |   |
| ---      | ---           |
| GLSL Shaders  | ---    | 
| **Single Resources:**
| ---   
| **Threaded Resources:**
| VkRenderPass  | renderPass | 
| VkFrameBuffer  | frameBuffer | 
| VkFrameBuffer  | frameBuffer2 (for stereo) | 
| **Descriptor Set Bindings:** (per render thread)
| ---
| **Descriptor Pools:** |
| ---

LineShader draws simple lines given in world coordinates. Each line has start point, end point and color. Fixed lines are stored in GPU at init phase and only read later. Dynamic lines are copied to GPU each frame (have ...Add in their name below)

| **LineShader** |   |
| ---      | ---           |
| GLSL Shaders  | vert, frag    | 
| **Single Resources:**
| VkShaderModule   | vert, frag | 
| VkBuffer   | vertexBuffer | 
| **Threaded Resources:**
| VkBuffer  | UBO for MVP: uniformBuffer | 
| VkBuffer  | UBO for MVP: uniformBuffer2 (for stereo) | 
| VkBuffer  | vertexBufferAdd
| VkRenderPass  | renderPass | 
| VkFrameBuffer  | frameBuffer | 
| VkFrameBuffer  | frameBuffer2 (for stereo) | 
| VkRenderPass  | renderPassAdd | 
| VkFrameBuffer  | frameBufferAdd | 
| VkFrameBuffer  | frameBufferAdd2 (for stereo) | 
| VkPipeline | graphicsPipeline
| VkPipeline | graphicsPipelineAdd
| **Descriptor Set Bindings:** (per render thread)
| Binding 0 | VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER VK_SHADER_STAGE_VERTEX_BIT
| **Descriptor Pools:** |
| poolSizeCount | 4
| maxSets | 5
