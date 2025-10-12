// common definitions shared by C++ and shaders

// Lights and joints max values
#define MAX_NUM_JOINTS 128
#define MAX_DYNAMIC_LIGHTS 4

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

#ifdef __cplusplus
struct PBRVertex {
	glm::vec3 pos; // 12 bytes
	float pad0;
	glm::vec3 normal; // 12 bytes
	float pad1;
	glm::vec2 uv0; // 8 bytes
	glm::vec2 uv1; // 8 bytes
	glm::uvec4 joint0; // 16 bytes, 4 joints per vertex
	glm::vec4 weight0; // 16 bytes, 4 weights per vertex
	glm::vec4 color; // 16 bytes, color in vertex structure
	//uint32_t pad[2]; // 8 bytes of padding to align the next member to 16 bytes. Do not use array on glsl side!!!

	// total size: 12 + 12 + 8 + 8 + 16 + 16 + 16 = 88 bytes

	bool operator==(const PBRVertex& other) const {
		return pos == other.pos &&
			normal == other.normal &&
			uv0 == other.uv0 &&
			uv1 == other.uv1 &&
			joint0 == other.joint0 &&
			weight0 == other.weight0 &&
			color == other.color;
	}
	PBRVertex(
		const glm::vec3& pos,
		const glm::vec3& normal,
		const glm::vec2& uv0,
		const glm::vec2& uv1 = glm::vec2(0.0f),
		const glm::uvec4& joint0 = glm::uvec4(0),
		const glm::vec4& weight0 = glm::vec4(0.0f),
		const glm::vec4& color = glm::vec4(1.0f)
	)
		: pos(pos), pad0(0.0f),
		normal(normal), pad1(0.0f),
		uv0(uv0), uv1(uv1),
		joint0(joint0), weight0(weight0), color(color)
	{
	}
	PBRVertex()
	{
	}
};
#else

struct PBRVertex {
    vec3 position;
    float pad0;
    vec3 normal;
    float pad1;
    vec2 uv0;
    vec2 uv1;
    uvec4 joint0;
    vec4 weight0;
    vec4 color0;
};
#endif
