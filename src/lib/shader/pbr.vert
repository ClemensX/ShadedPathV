#version 460
#extension GL_EXT_debug_printf : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord0;
layout(location = 2) in vec4 inColor0;

struct PBRTextureIndexes {
  uint baseColor;
};

layout (binding = 1) uniform UboInstance {
	mat4 model; 
    PBRTextureIndexes indexes;
} uboInstance;
 


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out PBRTextureIndexes textureIndexes;
layout(location = 3) out vec4 fragColor0;
layout(location = 4) out uint mode_out;

// sync with pbrPushConstants in pbrShader.h
layout(push_constant) uniform pbrPushConstants {
    uint mode; // 0: standard pbr metallicRoughness, 1: pre-light vertices with color in vertex structure
} push;

void main() {
    mode_out = push.mode;
    if (push.mode == 1) {
        //debugPrintfEXT("PBR vert color: %f %f %f %f\n", inColor0.x, inColor0.y, inColor0.z, inColor0.w);
		//fragColor0 = vec4(0, 1, 0, 1);
        fragColor0 = inColor0;
	}
    float val = ubo.model[0][0];
    if (val < 0.0 ) { // left
        fragColor0 = vec4(1, 0, 0, 1);
        mode_out = 1;
		//debugPrintfEXT("pbr render proj: %f\n", val);
	} else if (val == 0.0) {
        fragColor0 = vec4(0, 0, 1, 1);
    } else {
        fragColor0 = vec4(0, 1, 0, 1);
    }
    //fragColor0 = inColor0;
    //debugPrintfEXT("pbr render mode: %d\n", push.mode);
    //debugPrintfEXT("PBR:[0,0] [3,0] %f %f %f\n", uboInstance.model[0][0], uboInstance.model[3][0], uboInstance.model[0][2]);
    //debugPrintfEXT("PBR dynamic model matrix: %f %f %f\n", uboInstance.model[0][0], uboInstance.model[0][1], uboInstance.model[0][2]);
    //debugPrintfEXT("                        : %f %f %f\n", uboInstance.model[3][0], uboInstance.model[3][1], uboInstance.model[3][2]);
    //debugPrintfEXT("PBR input vertex world coord: %f %f %f\n", inPosition.x, inPosition.y, inPosition.z);
    gl_Position = ubo.proj * ubo.view * uboInstance.model * vec4(inPosition, 1.0);
    textureIndexes = uboInstance.indexes;
    //debugPrintfEXT("ubo.model 0 0 is %f\n", ubo.model[0][0]);
    //debugPrintfEXT("ubo.view 0 0 is %f\n", ubo.view[0][0]);
    //debugPrintfEXT("ubo.proj 0 0 is %f\n", ubo.proj[0][0]);

    //fragColor = inColor;
    fragColor = vec3(1, 1, 1);
    //debugPrintfEXT("final device coord PBR: %f %f %f\n", gl_Position.x, gl_Position.y, gl_Position.z);
    fragTexCoord = inTexCoord0;
}