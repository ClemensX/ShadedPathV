#version 460 
#extension GL_EXT_debug_printf:enable
#extension GL_EXT_nonuniform_qualifier : require

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

layout(set = 1, binding = 0) uniform sampler2D global_textures[];

// sync with BillboardPushConstants in BillboardShader.h
layout(push_constant) uniform BillboardPushConstants {
	float worldSizeOneEdge; // world size in meters, used for both dimensions (x and z)
    int heightmapTextureIndex;
} push;

void main()
{
	// heightmap start
    //outColor = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord);
    uint texIndex = push.heightmapTextureIndex;
    vec2 fragTexCoord = inPosition.xz;
//    float z = 0.5;
//    fragTexCoord = vec2(0.0, z);
//    float value0 = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord).r;
//    fragTexCoord = vec2(0.1, z);
//    float value1 = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord).r;
//    fragTexCoord = vec2(0.2, z);
//    float value2 = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord).r;
//    fragTexCoord = vec2(0.3, z);
//    float value3 = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord).r;
//    fragTexCoord = vec2(0.4, z);
//    float value4 = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord).r;
//    fragTexCoord = vec2(0.5, z);
//    float value5 = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord).r;
//    fragTexCoord = vec2(0.6, z);
//    float value6 = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord).r;
//    fragTexCoord = vec2(0.7, z);
//    float value7 = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord).r;
//    fragTexCoord = vec2(0.8, z);
//    float value8 = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord).r;
//    fragTexCoord = vec2(0.9, z);
//    float value9 = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord).r;
//    fragTexCoord = vec2(1.0, z);
//    float value10 = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord).r;
//    //debugPrintfEXT("float from R32_SFLOAT texture: %f %f %f %f %f %f %f %f %f %f %f \n", value0, value1, value2, value3, value4, value5, value6, value7, value8, value9, value10);

    // currently just use constant expressions: we have world xz from -1024 to 1024
    //float mappedx = (inPosition.x + 1024.0) / 2048.0;
    //debugPrintfEXT("pushed world size: %f\n", push.worldSizeOneEdge);
    float ws = push.worldSizeOneEdge;
    float wsHalf = ws / 2.0;
    // map world xz to (0.0 .. 1.0)
    float mappedx = (inPosition.x + wsHalf) / ws;
    float mappedz = (inPosition.z + wsHalf) / ws;
    //if (mappedx < 0.01)
    //debugPrintfEXT("mapped x and z: %f %f\n", mappedx, mappedz);
    fragTexCoord = vec2(mappedx, mappedz);
    float newHeight = texture(global_textures[nonuniformEXT(texIndex)], fragTexCoord).r;
    vec4 inP = inPosition;
    inP.y = newHeight;

    // heightmap end
    outType = inType;
    outQuat = inDirection;
    outWidth = inWidth;
    outHeight = inHeight;
    outIndex = inIndex;
    //gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);
    if (inType == 0) {
        gl_Position = ubo.view * vec4(inP.xyz, 1.0);
    } else if (inType == 1) {
       //debugPrintfEXT("bb vert.quat w x y z is %f %f %f %f\n", inDirection.w, inDirection.x, inDirection.y, inDirection.z);
       gl_Position = vec4(inP.xyz, 1.0);
    }
    //debugPrintfEXT("bb ubo.model 0 0 is %f\n", ubo.model[0][0]);
    if (inP.x < -1023) {
		//debugPrintfEXT("bb inPos x y z %f %f %f\n", inPosition.x, inPosition.y, inPosition.z);
	}
    //debugPrintfEXT("bb inPos x y z %f %f %f\n", inPosition.x, inPosition.y, inPosition.z);
    //debugPrintfEXT("bb inDir x y z %f %f %f\n", inDirection.x, inDirection.y, inDirection.z);
    //debugPrintfEXT("bb w h type %f %f %d\n", inWidth, inHeight, inType);
}
