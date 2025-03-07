#version 460
#extension GL_EXT_debug_printf : disable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV0;
layout(location = 2) in vec4 inColor0;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 baseColor;
} ubo;

struct PBRTextureIndexes {
  uint baseColor;
};

layout (binding = 1) uniform UboInstance {
	mat4 model; 
    PBRTextureIndexes indexes;
} uboInstance;
 


layout(location = 0) out vec4 vertexColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out PBRTextureIndexes textureIndexes;
layout(location = 3) out uint mode_out;

// sync with pbrPushConstants in pbrShader.h
layout(push_constant) uniform pbrPushConstants {
    uint mode; // 0: standard pbr metallicRoughness, 1: pre-light vertices with color in vertex structure
} push;

void main() {
    mode_out = push.mode;
    gl_Position = ubo.proj * ubo.view * uboInstance.model * vec4(inPos, 1.0);
    textureIndexes = uboInstance.indexes;
    //debugPrintfEXT("ubo.model 0 0 is %f\n", ubo.model[0][0]);

    vertexColor = inColor0 * ubo.baseColor;
    //fragColor = vec3(1, 1, 1);
    //debugPrintfEXT("final device coord PBR: %f %f %f\n", gl_Position.x, gl_Position.y, gl_Position.z);
    fragTexCoord = inUV0;
}