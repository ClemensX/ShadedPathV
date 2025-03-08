#version 460
#extension GL_EXT_debug_printf : disable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;
layout (location = 3) in vec2 inUV1;
layout (location = 4) in uvec4 inJoint0;
layout (location = 5) in vec4 inWeight0;
layout (location = 6) in vec4 inColor0;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 baseColor;
    vec3 camPos;
} ubo;

struct PBRTextureIndexes {
    uint baseColor;
    uint metallicRoughness;
    uint normal;
    uint occlusion;
    uint emissive;
};

#define MAX_NUM_JOINTS 128

// info for this model instance
// see 	struct PBRTextureIndexes and struct DynamicModelUBO in pbrShader.h
layout (binding = 1) uniform UboInstance {
    mat4 model; 
    mat4 jointMatrix[MAX_NUM_JOINTS];
    uint jointcount;
    uint pad0;
    uint pad1;
    uint pad2;
    //uint padding[2]; // 8 bytes of padding to align the next member to 16 bytes
    PBRTextureIndexes indexes;
} model_ubo;
 

layout(location = 0) out vec4 vertexColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out uint baseColorIndex;
layout(location = 3) out uint metallicRoughnessIndex;
layout(location = 4) out uint normalIndex;
layout(location = 5) out uint occlusionIndex;
layout(location = 6) out uint emissiveIndex;
layout(location = 7) out uint mode_out;

// sync with pbrPushConstants in pbrShader.h
layout(push_constant) uniform pbrPushConstants {
    uint mode; // 0: standard pbr metallicRoughness, 1: pre-light vertices with color in vertex structure
} push;

void main() {
    vec3 n = inNormal;
    vec2 uv = inUV1;
    uvec4 joint = inJoint0;
    //vec4 weight = inWeight0;
    mode_out = push.mode;
    gl_Position = ubo.proj * ubo.view * model_ubo.model * vec4(inPos, 1.0);
    //textureIndexes = model_ubo.indexes;
    //textureIndexes.baseColor = model_ubo.jointcount;
    //textureIndexes.baseColor = model_ubo.indexes.baseColor;
    //debugPrintfEXT("ubo.model tex index is %d\n", model_ubo.indexes.baseColor);
    //debugPrintfEXT("ubo.model tex index is %d %d %d %d\n", model_ubo.jointcount, model_ubo.pad0, model_ubo.pad1, model_ubo.pad2);

    baseColorIndex = model_ubo.indexes.baseColor;
    metallicRoughnessIndex = model_ubo.indexes.metallicRoughness;
    normalIndex = model_ubo.indexes.normal;
    occlusionIndex = model_ubo.indexes.occlusion;
    emissiveIndex = model_ubo.indexes.emissive;

//    test indexes
//    baseColorIndex = model_ubo.indexes.metallicRoughness;
//    metallicRoughnessIndex = model_ubo.indexes.baseColor;

    vertexColor = inColor0 * ubo.baseColor;
    fragTexCoord = inUV0;
    vertexColor = inColor0 * ubo.baseColor;
    //fragColor = vec3(1, 1, 1);
    //debugPrintfEXT("final device coord PBR: %f %f %f\n", gl_Position.x, gl_Position.y, gl_Position.z);
    fragTexCoord = inUV0;
}