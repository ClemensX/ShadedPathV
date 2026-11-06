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

// LOD definitions:

#define LOD_CATEGORY_GENERAL 0
#define LOD_CATEGORY_SMALL_GRASS 1
#define LOD_CATEGORY_SIMPLE_STONE 2
#define LOD_CATEGORY_INVISIBLE 100 // used to mark objects too far away to render

#ifdef __cplusplus

struct LodCategoryDef {
	int id;
	std::string_view name;
};
// strings are not usable in shaders!!
inline constexpr LodCategoryDef LOD_CATEGORIES[] = {
	{ LOD_CATEGORY_GENERAL,      "general" },
	{ LOD_CATEGORY_SMALL_GRASS,  "small grass" },
	{ LOD_CATEGORY_SIMPLE_STONE, "simple stone" },
	{ LOD_CATEGORY_INVISIBLE,    "invisible" },
};
#endif

struct LodEntry {
    int category;
	int max_index_used; // the highest index in dist_map being used (== lowest available lod)
    float minDistance;  // if >0 means disable view if too close ot within object boundaries
	float maxDistance;  // invisible if distance it greater than this 
    float dist_map[10]; // map distance to LOD level, 10 levels max, -1 indicates no LOD for this distance
};

#define LOD_ENTRIES_COUNT 3

#ifdef __cplusplus
inline constexpr
#endif
LodEntry LOD_ENTRIES[LOD_ENTRIES_COUNT] = {
    { LOD_CATEGORY_GENERAL, 9, 0.0f, 1000.0f,{ 0, 1, 5, 10, 15, 25, 30, 50, 70, 150 } },
    { LOD_CATEGORY_SMALL_GRASS, 4, 0.0f, 70.0f,{ 0, 15, 40, 50, 60, -1, -1, -1, -1, -1 } },
    { LOD_CATEGORY_SIMPLE_STONE, 4, 0.0f, 700.0f,{ 0, 70, 170, 300, 500, -1, -1, -1, -1, -1 } }
};

#ifdef __cplusplus
inline
#endif
int calculateLODIndex(int lod_category, float distance) {
	if (distance >= LOD_ENTRIES[lod_category].maxDistance) {
		return -1; // do not render this object at all
	}
	// we go back from max_index_used to 0 and find the first entry where the distance is greater
	int max_index = LOD_ENTRIES[lod_category].max_index_used;
	for (int i = max_index; i >= 0; --i) {
		float v = LOD_ENTRIES[lod_category].dist_map[i];
        if (v < 0) {
			return -1; // we reached a too small distance to display this object 
        }
		//if (v < 0) debugPrintfEXT("TASK SHADER: ERROR: invalid fixed_lod value %f for category %u index %d\n", v, lod_category, i);
		if (distance >= v) {
			return i;
		}
	}
	return 0;
}


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
