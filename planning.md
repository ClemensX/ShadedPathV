# Planning

## Flora for World Creator Forest 001 sample:

### Create Assets

| **Asset** | LODs | Meshlets | LOD Selection
| ---       | ---  | ---      | ---
| Grass_C   |   x   |   x       | x
| Grass_B   |	  |          |
| DropSeed_C   |	  |          |
| DropSeed_B   |	  |          |
| Bush_A   |	  |          |
| Bush_A(?)   |	  |          |
| Acacia_B   |	  |          |
| Acacia_A   |	  |          |

# Old content - remove later
## Vulkan Resources Use

Current use of Vulkan resources per shader:

## SimpleShader

SimpleShader is a playfield to try out rendering techniques:
a single texture is rendered in 2 rectangles that keep rotating

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

## ClearShader

Clear Shader is not a complete shader and should always be first in list of used shaders. It clears color and depth attachement

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

## LineShader

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

## CubeShader

CubeShader either draws a skybox or a cube from a CubeMap (viewType VK_IMAGE_VIEW_TYPE_CUBE)

| **CubeShader** |   |
| ---      | ---           |
| GLSL Shaders  | vert, frag    | 
| **Single Resources:**
| VkShaderModule   | vert, frag | 
| **Threaded Resources:**
| VkBuffer  | UBO for MVP: uniformBuffer | 
| VkBuffer  | UBO for MVP: uniformBuffer2 (for stereo) | 
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

## PBRShader

PBRShader draws objects read from glTF files with PBR lighing

| **PBRShader** |   |
| ---      | ---           |
| GLSL Shaders  | vert, frag    | 
| **Single Resources:**
| VkShaderModule   | vert, frag | 
| **Threaded Resources:**
| VkBuffer  | UBO for MVP: uniformBuffer | 
| VkBuffer  | UBO for MVP: uniformBuffer2 (for stereo) | 
| VkBuffer  | Model Matrix for each object: dynamicUniformBuffer
| VkRenderPass  | renderPass | 
| VkFrameBuffer  | frameBuffer | 
| VkFrameBuffer  | frameBuffer2 (for stereo) | 
| VkPipeline | graphicsPipeline
| **Descriptor Set Bindings:** (per render thread)
| Binding 0 | VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER VK_SHADER_STAGE_VERTEX_BIT
| Binding 1 | VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC VK_SHADER_STAGE_VERTEX_BIT
| Binding 2 | VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER VK_SHADER_STAGE_FRAGMENT_BIT
| **Descriptor Pools:** |
| poolSizeCount | 9
| maxSets | 1005 (MaxObjects + 5)

## BillboardShader

BillboardShader draws simple billboards in world coordinates with direction and size

| **BillboardShader** |   |
| ---      | ---           |
| GLSL Shaders  | vert, geom, frag    | 
| **Single Resources:**
| VkShaderModule   | vert, geom, frag | 
| **Threaded Resources:**
| VkBuffer  | UBO for MVP: uniformBuffer | 
| VkBuffer  | UBO for MVP: uniformBuffer2 (for stereo) | 
| VkRenderPass  | renderPass | 
| VkFrameBuffer  | frameBuffer | 
| VkFrameBuffer  | frameBuffer2 (for stereo) | 
| VkPipeline | graphicsPipeline
| **Descriptor Set Bindings:** (per render thread)
| Binding 0 | VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER VK_SHADER_STAGE_VERTEX_BIT \| VK_SHADER_STAGE_GEOMETRY_BIT
| Binding 1 | VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER VK_SHADER_STAGE_FRAGMENT_BIT
| **Descriptor Pools:** |
| poolSizeCount | 6
| maxSets | 5

## UIShader

UIShader will not be covered here - it is vastly different from the other shaders and there are no plans to unify it with the others