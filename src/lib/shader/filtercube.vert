/* Copyright (c) 2018-2023, Sascha Willems (orig. version)
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 460
#extension GL_EXT_debug_printf : disable

//layout (location = 0) in vec3 inPos;

layout(push_constant) uniform PushConsts {
	layout (offset = 0) mat4 mvp;
	layout (offset = 64) uint textureIndex;
} pushConsts;

layout (location = 0) out vec3 outUVW;

const vec3 vertices[24] = vec3[](
  vec3(-0.5, -0.5, 0.5),
  vec3(0.5, -0.5, 0.5),
  vec3(-0.5, 0.5, 0.5),
  vec3(0.5, 0.5, 0.5),
  vec3(0.5, -0.5, 0.5),
  vec3(-0.5, -0.5, 0.5),
  vec3(0.5, -0.5, -0.5),
  vec3(-0.5, -0.5, -0.5),
  vec3(0.5, 0.5, 0.5),
  vec3(0.5, -0.5, 0.5),
  vec3(0.5, 0.5, -0.5),
  vec3(0.5, -0.5, -0.5),
  vec3(-0.5, 0.5, 0.5),
  vec3(0.5, 0.5, 0.5),
  vec3(-0.5, 0.5, -0.5),
  vec3(0.5, 0.5, -0.5),
  vec3(-0.5, -0.5, 0.5),
  vec3(-0.5, 0.5, 0.5),
  vec3(-0.5, -0.5, -0.5),
  vec3(-0.5, 0.5, -0.5),
  vec3(-0.5, -0.5, -0.5),
  vec3(-0.5, 0.5, -0.5),
  vec3(0.5, -0.5, -0.5),
  vec3(0.5, 0.5, -0.5)
);

const int indices[36] = int[](
  0, 1, 2,
  3, 2, 1,
  4, 5, 6,
  7, 6, 5,
  8, 9, 10,
  11, 10, 9,
  12, 13, 14,
  15, 14, 13,
  16, 17, 18,
  19, 18, 17,
  20, 21, 22,
  23, 22, 21
);


void main() 
{
    vec3 inPos = vertices[indices[gl_VertexIndex]];
    outUVW = inPos;
    //debugPrintfEXT("gl_VertexID: %d\n", gl_VertexIndex);
    //debugPrintfEXT("index %d  vert: %f %f %f\n", gl_VertexIndex, inPos.x, inPos.y, inPos.z);
    gl_Position = pushConsts.mvp * vec4(inPos, 1.0);
    //gl_Position = vec4(inPos.x + .5, inPos.y + .5, inPos.z + .5, 1.0);
    //debugPrintfEXT("gl_Position %f %f %f\n", gl_Position.x, gl_Position.y, gl_Position.z, gl_Position.w);
//
//	outUVW = inPos;
//	debugPrintfEXT("bin do, hajo\n");
//	debugPrintfEXT("gl_VertexIndex: %d\n", gl_VertexIndex);
//	gl_Position = pushConsts.mvp * vec4(inPos.xyz, 1.0);
}
