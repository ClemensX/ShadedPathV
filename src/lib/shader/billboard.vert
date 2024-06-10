#version 460 
#extension GL_EXT_debug_printf:enable
#extension GL_KHR_vulkan_glsl:enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inDirection;
layout(location = 2) in float inWidth;
layout(location = 3) in float inHeight;
layout(location = 4) in uint inType; // billboard type: 0 is towards camera, 1 is absolute inDirection
layout(location = 5) in uint inIndex; // global texture index

layout(location = 1) out uint outType;
layout(location = 2) out vec4 outQuat;
layout(location = 3) out float outWidth;
layout(location = 4) out float outHeight;
layout(location = 5) out uint outIndex;

void main()
{
    outType = inType;
    outQuat = inDirection;
    outWidth = inWidth;
    outHeight = inHeight;
    outIndex = inIndex;
    //gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);
    if (inType == 0) {
        gl_Position = ubo.view * vec4(inPosition.xyz, 1.0);
    } else if (inType == 1) {
       //debugPrintfEXT("bb vert.quat w x y z is %f %f %f %f\n", inDirection.w, inDirection.x, inDirection.y, inDirection.z);
       gl_Position = vec4(inPosition.xyz, 1.0);
    }
    //debugPrintfEXT("bb ubo.model 0 0 is %f\n", ubo.model[0][0]);
    //debugPrintfEXT("bb inPos x y z %f %f %f\n", inPosition.x, inPosition.y, inPosition.z);
    //debugPrintfEXT("bb inDir x y z %f %f %f\n", inDirection.x, inDirection.y, inDirection.z);
    //debugPrintfEXT("bb w h type %f %f %d\n", inWidth, inHeight, inType);
}
