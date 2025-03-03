/* Copyright (c) 2018-2023, Sascha Willems (orig. version)
 *
 * SPDX-License-Identifier: MIT
 *
 */

#version 460
#extension GL_EXT_debug_printf : enable

//layout (location = 0) in vec3 inPos;

layout(push_constant) uniform PushConsts {
	layout (offset = 0) mat4 mvp;
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
    debugPrintfEXT("gl_VertexID: %d\n", gl_VertexIndex);
    gl_Position = pushConsts.mvp * vec4(inPos, 1.0);
//
//	outUVW = inPos;
//	debugPrintfEXT("bin do, hajo\n");
//	debugPrintfEXT("gl_VertexIndex: %d\n", gl_VertexIndex);
//	gl_Position = pushConsts.mvp * vec4(inPos.xyz, 1.0);
}
