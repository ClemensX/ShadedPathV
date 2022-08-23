#version 460
#extension GL_EXT_debug_printf:enable
#extension GL_KHR_vulkan_glsl:enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	float bloat;
	bool outside;
} ubo;

layout(location = 0) in vec3 inPosition;
//layout(location = 1) out float PointSize;

void main()
{
    gl_PointSize = 1.0;
    //debugPrintfEXT("ubo.model 0 0 is %f\n", ubo.model[0][0]);
}
