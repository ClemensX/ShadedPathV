// common definitions shared by C++ and shaders

// see https://github.com/nvpro-samples/gl_vk_meshlet_cadscene/blob/master/common.h
#ifndef GLEXT_MESHLET_VERTEX_COUNT
// primitive count should be 40, 84 or 126
//    vertex count should be 32 or 64
//
// 64 vertices &  84 triangles:
//    works typically well for NV
// 64 vertices &  64 triangles:
//    is more portable for EXT usage
//    (hw that does 128 & 128 well, can do 2 x 64 & 64 at once)
// 64 vertices & 126 triangles:
//    can work in z-only or other very low extra
//    vertex attribute scenarios for NV
//
#define GLEXT_MESHLET_VERTEX_COUNT 64
#define GLEXT_MESHLET_PRIMITIVE_COUNT 126
// must be multiple of SUBGROUP_SIZE
#define GLEXT_MESHLET_PER_TASK 32
#endif

#ifndef GLEXT_MESHLET_ENCODING
#define GLEXT_MESHLET_ENCODING NVMESHLET_ENCODING_PACKBASIC
#endif
