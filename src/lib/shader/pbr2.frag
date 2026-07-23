#version 450
#extension GL_EXT_debug_printf : enable
#extension GL_EXT_mesh_shader : enable
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_buffer_reference_uvec2 : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
#extension GL_EXT_nonuniform_qualifier : require


#include "common_cpp_shader.h"
//#include "shadermaterial.glsl"
#include "pbr_mesh_common2.glsl"

layout(location = 0) flat in PBRVertexOutFlat inVertFlat;
layout(location = 1) in PBRVertexOut inVert; // flat removed
// mode 0: pbr metallic roughness
// mode 1: only use vertex color

vec2 inUV0 = inVert.uv0;
vec2 inUV1 = inVert.uv1;
vec3 inWorldPos = inVert.worldPos;
vec3 inNormal = inVert.normal;
vec3 camPos = ubo.camPos;
vec4 inColor0 = inVert.color0;
float inpad0 = inVert.pad0;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform samplerCube global_textures3d[];
layout(set = 1, binding = 0) uniform sampler2D global_textures2d[];

vec4 textureBindless3DLod(uint textureid, vec3 uv, float lod) {
	vec3 myuv = uv;
	//myuv.y = 1.0 - myuv.y;
	return textureLod(global_textures3d[nonuniformEXT(textureid)], myuv, lod);
}

vec4 textureBindless3D(uint textureid, vec3 uv) {
	return texture(global_textures3d[nonuniformEXT(textureid)], uv);
}

vec4 textureBindless2D(uint textureid, vec2 uv) {
	vec2 wrappedUV = uv;
	return texture(global_textures2d[nonuniformEXT(textureid)], wrappedUV);
}


void main() {
	outColor = vec4(1, 1, 1, 1);
}
