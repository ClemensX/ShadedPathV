#version 460
#extension GL_EXT_debug_printf : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord0;

layout (binding = 1) uniform UboInstance {
	mat4 model; 
} uboInstance;
 


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    //debugPrintfEXT("PBR:[0,0] [3,0] %f %f %f\n", uboInstance.model[0][0], uboInstance.model[3][0], uboInstance.model[0][2]);
    //debugPrintfEXT("PBR dynamic model matrix: %f %f %f\n", uboInstance.model[0][0], uboInstance.model[0][1], uboInstance.model[0][2]);
    //debugPrintfEXT("                        : %f %f %f\n", uboInstance.model[3][0], uboInstance.model[3][1], uboInstance.model[3][2]);
    //debugPrintfEXT("PBR input vertex world coord: %f %f %f\n", inPosition.x, inPosition.y, inPosition.z);
    gl_Position = ubo.proj * ubo.view * uboInstance.model * vec4(inPosition, 1.0);
    //debugPrintfEXT("ubo.model 0 0 is %f\n", ubo.model[0][0]);
    //debugPrintfEXT("ubo.view 0 0 is %f\n", ubo.view[0][0]);
    //debugPrintfEXT("ubo.proj 0 0 is %f\n", ubo.proj[0][0]);

    //fragColor = inColor;
    fragColor = vec3(1, 1, 1);
    //debugPrintfEXT("final device coord PBR: %f %f %f\n", gl_Position.x, gl_Position.y, gl_Position.z);
    fragTexCoord = inTexCoord0;
}