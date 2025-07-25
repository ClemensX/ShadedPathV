#pragma once

class Util;
struct SoundDef;

enum class MeshFlags : int {
	MESH_TYPE_INVALID = 0,
	MESH_TYPE_PBR = 1,
	MESH_TYPE_SKINNED = 2,
	MESH_TYPE_NO_TEXTURES = 4,
    MESH_TYPE_FLIP_WINDING_ORDER = 8, // flip clockwise <-> counter-clockwise winding order
	MESHLET_DEBUG_COLORS = 16, // apply vertex color to all triangles of one meshlet
	MESH_TYPE_COUNT = -1 // always last
};

class MeshFlagsCollection {
private:
	std::bitset<32> flags;

public:
	MeshFlagsCollection() : flags(0) {}
	MeshFlagsCollection(MeshFlags flag) : flags(0) {
        setFlag(flag);
	}

	void setFlag(MeshFlags flag) {
		flags.set(static_cast<size_t>(flag));
	}

	void clearFlag(MeshFlags flag) {
		flags.reset(static_cast<size_t>(flag));
	}

	bool hasFlag(MeshFlags flag) const {
		return flags.test(static_cast<size_t>(flag));
	}
};

// all meshes loaded from one gltf file. Textures are maintained here, meshes are contained
struct MeshCollection {
        std::string id;
        std::string filename;
        bool available = false; // true if this object is ready for use in shader code
        std::vector<ktxTexture*> textureParseInfo;
        std::vector<::TextureInfo*> textureInfos;
        std::vector<MeshInfo*> meshInfos;
		MeshFlagsCollection flags;
};

// meshlet class holds data for a single meshlet, contains collection of vertices and primitives (triangles).
// vertex buffer is unchanged, all meshlet operations work on indices only
class Meshlet {
public:
	Meshlet() {
		verticesIndices.reserve(256);
		primitives.reserve(256);
	}
	Meshlet(const Meshlet&) = default;
	Meshlet(Meshlet&&) = default;
	Meshlet& operator=(const Meshlet&) = default;
	Meshlet& operator=(Meshlet&&) = default;

	// debug info:
    bool debugColors = false; // if true, color all triangles of this meshlet with the same color
	// indices of the meshlet (index into global vertex buffer)
	// actual vertex coord are globalVertexBuffer[verticesIndices[i]], if i is local 8 bit index
    std::vector<uint32_t> verticesIndices; // actual index buffer, map meshlet vert index 0..255 to global index
	using uvec3 = std::array<uint32_t, 3>;
	std::vector<uvec3> primitives;
	struct MeshletVertInfo;
	struct MeshletTriangle {
		// local indices of the meshlet, fit into one byte
		//uint8_t a, b, c; // vertices, defined by local index into verticesIndices
		std::vector<MeshletVertInfo*> vertices;
		std::vector<MeshletTriangle*> neighbours;
		float centroid[3]{};
		uint32_t id;
		uint32_t flag = -1;
		uint32_t dist;
	};
	struct MeshletVertInfo {
		std::vector<MeshletTriangle*> neighbours; // vertex index
		unsigned int index;
		unsigned int degree;
	};

	// meshletmaker: https://github.com/Senbyo/meshletmaker
	static void applyMeshletAlgorithmGreedyVerts(
		std::unordered_map<uint32_t, Meshlet::MeshletVertInfo>& indexVertexMap, // 117008
		std::vector<Meshlet::MeshletVertInfo*>& vertsVector, // 117008
		std::vector<Meshlet::MeshletTriangle*>& triangles, // 231256
		std::vector<Meshlet>& meshlets, // 0
		const std::vector<PBRShader::Vertex>& vertexBuffer, // 117008 vertices
		uint32_t primitiveLimit, uint32_t vertexLimit // 125, 64
	);

	bool empty() const { return verticesIndices.empty(); }

	void reset() {
		verticesIndices.clear(); // clear but retain allocated memory
		primitives.clear();
	}

	// check if cache can hold one more triangle
	bool cannotInsert(const uint32_t* indices, uint32_t maxVertexSize, uint32_t maxPrimitiveSize) const
	{
		// skip degenerate
		if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2])
		{
			return false;
		}

		uint32_t found = 0;

		// check if any of the incoming three indices are already in meshlet
		for (auto vert_idx : verticesIndices) {
			for (int i = 0; i < 3; ++i) {
				uint32_t idx = indices[i];
				if (vert_idx == idx) {
					found++;
				}
			}
		}
		// out of bounds
		bool ret = (verticesIndices.size() + 3 - found) > maxVertexSize || (primitives.size() + 1) > maxPrimitiveSize;
		if (ret) {
			assert(verticesIndices.size() <= maxVertexSize);
			assert(primitives.size() <= maxPrimitiveSize);
		}
		return ret;
	}

	// insert new triangle
	void insert(const uint32_t* indices) {
		uint32_t triangle[3];

		// skip degenerate
		if (indices[0] == indices[1] || indices[0] == indices[2] || indices[1] == indices[2])
		{
			return;
		}

		for (int i = 0; i < 3; ++i) {
			// take out an index
			uint32_t idx = indices[i];
			bool found = false;

			// check if idx is already in cache
			for (uint32_t v = 0; v < verticesIndices.size(); ++v) {
				if (idx == verticesIndices[v])
				{
					triangle[i] = v;
					found = true;
					break;
				}
			}
			// if idx is not in cache add it
			if (!found)
			{
				verticesIndices.push_back(idx);
				//actualVertices[numVertices] = verts[idx]; we do not have actual vertices here, only indices
				triangle[i] = verticesIndices.size() - 1;

				//if (numVertices)
				//{
				//	numVertexDeltaBits = std::max(findMSB((idx ^ vertices[0]) | 1) + 1, numVertexDeltaBits);
				//}
				//numVertexAllBits = std::max(numVertexAllBits, findMSB(idx) + 1);

				//numVertices++;
			}
		}

        uvec3 tri = { triangle[0], triangle[1], triangle[2] };
        primitives.push_back(tri);
	}
};

// input for meshlet calculations, basically the raw data from glTF:
struct MeshletIn {
	const std::vector<PBRShader::Vertex>& vertices;
	const std::vector<uint32_t>& indices;
	const uint32_t primitiveLimit; // usually 126
	const uint32_t vertexLimit; // usually 64
};

// intermediate structures used during meshlet calculations
// can be cleared after meshlet calculations are done
struct MeshletIntermediate {
	std::vector<Meshlet::MeshletTriangle>& trianglesVector; // base storage for triangles
	std::unordered_map<uint32_t, Meshlet::MeshletVertInfo>& indexVertexMap;
	std::vector<Meshlet::MeshletTriangle*>& triangles;
	std::vector<Meshlet::MeshletVertInfo*>& vertsVector; // meshlet vertex info, used to store vertex indices and neighbours
	std::vector<uint32_t>& meshletVertexIndices; // indices into vertices
};

struct MeshletOut {
	std::vector<Meshlet>& meshlets;
	// output: needed on GPU side
	std::vector<PBRShader::PackedMeshletDesc>& outMeshletDesc;
	std::vector<uint8_t>& outLocalIndexPrimitivesBuffer;   // local indices for primitives (3 indices per triangle)
	std::vector<uint32_t>& outGlobalIndexBuffer; // vertex indices into vertex buffer
	//std::vector<Meshlet::MeshletVertInfo> indexVertexMap; // 117008
	//std::vector<Meshlet::MeshletVertInfo*> vertsVector; // 117008
	//std::vector<Meshlet::MeshletTriangle*> triangles; // 231256
};

struct BoundingBoxCorners {
	glm::vec3 corners[8];
};


enum class Axis { X, Y, Z };

// Lexicographical comparison for glm::vec3
inline bool lessVec3(const glm::vec3& a, const glm::vec3& b) {
	if (a.x != b.x) return a.x < b.x;
	if (a.y != b.y) return a.y < b.y;
	return a.z < b.z;
}

// Sort vertices by the position (first x, then y, then z). afterwards all equal positions are grouped together
template<typename T>
void sortByPos(std::vector<T>& vertices) {
	std::sort(vertices.begin(), vertices.end(),
		[](const T& lhs, const T& rhs) {
			return lessVec3(lhs.pos, rhs.pos);
		}
	);
}

// Sort vector<Meshlet::MeshletVertInfo*> by the position in the base vertex buffer, along the given axis.
template<typename T>
void sortByPosAxis(std::vector<Meshlet::MeshletVertInfo*>& verticesMeshletInfoVector, const std::vector<T>& verticesBaseVector, Axis axis) {
	std::sort(verticesMeshletInfoVector.begin(), verticesMeshletInfoVector.end(),
		[&verticesBaseVector, axis](const Meshlet::MeshletVertInfo* lhs, const Meshlet::MeshletVertInfo* rhs) {
			const glm::vec3& posL = verticesBaseVector[lhs->index].pos;
			const glm::vec3& posR = verticesBaseVector[rhs->index].pos;
			switch (axis) {
			case Axis::X: return posL.x < posR.x;
			case Axis::Y: return posL.y < posR.y;
			case Axis::Z: return posL.z < posR.z;
			default:      return false;
			}
		}
	);
}

// Sort vector<Meshlet::MeshletVertInfo*> by the position in the base vertex buffer, along the given axis.
template<typename T>
void sortTrianglesByPosAxis(std::vector<Meshlet::MeshletTriangle*>& triangles, const std::vector<T>& verticesBaseVector, Axis axis) {
	std::sort(triangles.begin(), triangles.end(),
		[&verticesBaseVector, axis](const Meshlet::MeshletTriangle* lhs, const Meshlet::MeshletTriangle* rhs) {
			float l, r;
			switch (axis) {
            case Axis::X: l = lhs->centroid[0]; r = rhs->centroid[0]; break;
            case Axis::Y: l = lhs->centroid[1]; r = rhs->centroid[1]; break;
            case Axis::Z: l = lhs->centroid[2]; r = rhs->centroid[2]; break;
			default:      return false;
			}
            return l < r;
		}
	);
}

// Describe a single loaded mesh. mesh IDs are unique, several Objects may be instantiated backed by the same mesh
struct MeshInfo
{
	std::string id;
	bool available = false; // true if this object is ready for use in shader code
	MeshFlagsCollection flags;
    uint32_t meshletVerticesLimit = 0; // limit for meshlet vertices, we need to a fixed value for index buffer alignment

	// gltf data: valid after object load, should be cleared after upload
	std::vector<PBRShader::Vertex> vertices;
	std::vector<uint32_t> indices;
    std::vector<uint32_t> meshletVertexIndices; // indices into vertices, used for meshlets
    std::vector<Meshlet> meshlets; // meshlets for this mesh, for use in MeshShader
    std::vector<Meshlet::MeshletVertInfo*> vertsVector; // meshlet vertex info, used to store vertex indices and neighbours
	// output: needed on GPU side
	std::vector<PBRShader::PackedMeshletDesc> outMeshletDesc;
	std::vector<uint8_t> outLocalIndexPrimitivesBuffer;   // local indices for primitives (3 indices per triangle)
	std::vector<uint32_t> outGlobalIndexBuffer; // vertex indices into vertex buffer

	// named accessors for textures in above vector:
	::TextureInfo* baseColorTexture = nullptr;
	::TextureInfo* metallicRoughnessTexture = nullptr;
	::TextureInfo* normalTexture = nullptr;
	::TextureInfo* occlusionTexture = nullptr;
	::TextureInfo* emissiveTexture = nullptr;

	// GPU data:
	VkBuffer vertexBuffer = nullptr;
	VkDeviceMemory vertexBufferMemory = nullptr;
	VkBuffer indexBuffer = nullptr;
	VkDeviceMemory indexBufferMemory = nullptr;
	VkBuffer meshletDescBuffer = nullptr;
	VkDeviceMemory meshletDescBufferMemory = nullptr;
	VkBuffer globalIndexBuffer = nullptr;
	VkDeviceMemory globalIndexBufferMemory = nullptr;
	VkBuffer localIndexBuffer = nullptr;
	VkDeviceMemory localIndexBufferMemory = nullptr;
	VkBuffer vertexStorageBuffer = nullptr;
	VkDeviceMemory vertexStorageBufferMemory = nullptr;
	//VkDescriptorSet descriptorSet = nullptr;

	// gltf mesh index, only valid during gltf parsing. -1 if not yet set
	int gltfMeshIndex = -1;
	// base transform from gltf file (default to identity)
	glm::mat4 baseTransform = glm::mat4(1.0f);

	// link back to collection
	MeshCollection* collection = nullptr;
	PBRShader::ShaderMaterial material;
    // gltf loaded models are (currently) always PBR metallic roughness
	bool isMetallicRoughness() {
		return metallicRoughness;
	}
	// during gltf model load, this flag is enabled. do not call from somwehere else
	bool metallicRoughness = false;
	// copy gltf material info here, may also be overwritten in app code
	bool isDoubleSided = false;

	// get bounding box either from mesh data, will only be called once
	void getBoundingBox(BoundingBox& box);
	
	void logInfo() const {
        Log("Mesh ID: " << id << "\n");
        if (baseColorTexture) {
            Log("Base Color Texture ID: " << baseColorTexture->id << ", Index: " << baseColorTexture->index << "\n");
        }
        if (metallicRoughnessTexture) {
            Log("Metallic Roughness Texture ID: " << metallicRoughnessTexture->id << ", Index: " << metallicRoughnessTexture->index << "\n");
        }
        if (normalTexture) {
            Log("Normal Texture ID: " << normalTexture->id << ", Index: " << normalTexture->index << "\n");
        }
        if (occlusionTexture) {
            Log("Occlusion Texture ID: " << occlusionTexture->id << ", Index: " << occlusionTexture->index << "\n");
        }
        if (emissiveTexture) {
            Log("Emissive Texture ID: " << emissiveTexture->id << ", Index: " << emissiveTexture->index << "\n");
        }
        Log("Number of Vertices: " << vertices.size() << "\n");
        Log("Number of Indices: " << indices.size() << "\n");
    }
	bool boundingBoxAlreadySet = false;
	BoundingBox boundingBox;
};
typedef MeshInfo* ObjectID;

// Bitfield for controlling meshlet behaviour
enum class MeshletFlags : uint32_t {
	MESHLET_SORT = 1,
	MESHLET_ALG_SIMPLE = 2,
	MESHLET_ALG_GREEDY_VERT = 4,
    MESHLET_SIMPLIFY_MESH = 8, // remove duplicate vertices and triangles
};

// Mesh Store to organize objects loaded from gltf files.
class MeshStore {
public:
	// init object store
	void init(ShadedPathEngine* engine);
	~MeshStore();
	// load mesh wireframe and add to Line vector (only for first mesh in gltf file)
	void loadMeshWireframe(std::string filename, std::string id, std::vector<LineDef>& lines);
	// load meshes from glTF file, objects are referenced via id string according to this schema:
	// ref string == gltf mesh
	// =======================
	// id == mesh[0]
	// id.gltf_mesh_name == mesh with name == gltf_mesh_name
	// id.2 == mesh[2]
	void loadMesh(std::string filename, std::string id, MeshFlagsCollection flags = MeshFlagsCollection());
	// get sorted object list (sorted by type)
	// meshes are only resorted if one was added in the meantime
	const std::vector<MeshInfo*> &getSortedList();
	// upload single model to GPU
	void uploadObject(MeshInfo* obj);
	// initialize MeshInfo, also add to collection. id is expected to be in collection format like myid.2
	// myid.0 is a synonym for myid
	MeshInfo* initMeshInfo(MeshCollection* coll, std::string id);

	MeshInfo* getMesh(std::string id);
	// to render an object using meshlets we need:
    // 1. meshlet desc buffer, most important: get global index start for each meshlet
    // 2. global index buffer, which contains indices into the global vertex buffer
    // 3. local index buffer, byte buffer which maps local meshlet vertex index to global index buffer: byte val + global index start is the index where the actual vertex is found
	void calculateMeshletsX(std::string id, uint32_t vertexLimit = 64, uint32_t primitiveLimit = 126);
	void calculateMeshlets(std::string id, uint32_t meshlet_flags, uint32_t vertexLimit = GLEXT_MESHLET_VERTEX_COUNT, uint32_t primitiveLimit = GLEXT_MESHLET_PRIMITIVE_COUNT);
	const float VERTEX_REUSE_THRESHOLD = 1.3f; // if vertex position duplication ratio is greater than this, log a warning
	// IMPORTANT: this is no longer true! We need vertex normals for PBR and they should not be removed!
    // comment is left here for reference
    // check if loaded mesh has many vertices that are identical in position, color and uv coords but differ in normal direction.
    // this is a common problem with glTF files that were exported from Blender, where the normals are not recomputed.
    // �	In edit mode, select all vertices of the mesh, then press "Alt+N" to display normal menu, then merge normals. 
	//      you might want to add normals view in mesh edit mode overlay (use "Display Split Normals")
	// �	Export Settings : When exporting(e.g., to glTF), ensure normals are exported and modifiers are applied
    void checkVertexNormalConsistency(std::string id);
	// debug graphics, usually means bounding box and normals are added to line shader
	void debugGraphics(WorldObject* obj, FrameResources& fr, glm::mat4 modelToWorld, glm::vec4 color = Colors::Red, float normalLineLength = 0.001f);
	// apply fixed colors to all vertices of one meshlet (useful for debugging)
	// may not be totally correct if some vertices are shared between meshlets (color value will be overwritten)
	void applyDebugMeshletColorsToVertices(MeshInfo* mesh);
	// apply same color to all trtiangles of the meshlets (useful for debugging)
    // this simply markes the meshlet with a debug flag, the actual color is applied in the shader
	void applyDebugMeshletColorsToMeshlets(MeshInfo* mesh);
	// draw object from lines using its meshlet information only
    // also servers as a debug function to visualize meshlets and to document meshlet structure
	void debugRenderMeshlet(WorldObject* obj, FrameResources& fr, glm::mat4 modelToWorld, glm::vec4 color = Colors::Red);
	void debugRenderMeshletFromBuffers(WorldObject* obj, FrameResources& fr, glm::mat4 modelToWorld, glm::vec4 color = Colors::Red);
	void debugRenderMeshletFromBuffers(FrameResources& fr, glm::mat4 modelToWorld,
		std::vector<PBRShader::PackedMeshletDesc>& meshletDesc,
		std::vector<uint8_t>& localIndexPrimitivesBuffer,
		std::vector<uint32_t>& globalIndexBuffer,
		std::vector<PBRShader::Vertex>& vertices
	);
    void logMeshletStats(MeshInfo* mesh);

private:
	MeshCollection* loadMeshFile(std::string filename, std::string id, std::vector<std::byte> &fileBuffer, MeshFlagsCollection flags);
	std::unordered_map<std::string, MeshInfo> meshes;
	std::vector<MeshCollection> meshCollections;
	ShadedPathEngine* engine = nullptr;
	Util* util = nullptr;
	std::vector<MeshInfo*> sortedList;
	glTF gltf;
	// iterate through index buffer and place a fixed number of triangles into meshlets
	static void applyMeshletAlgorithmSimple(MeshletIn& in, MeshletOut& out);
	// iterate through index buffer and place a fixed number of triangles into meshlets
	static void applyMeshletAlgorithmSimpleOnSortedTriangles(MeshletIn& in, MeshletIntermediate& temp, MeshletOut& out);
	// meshletmaker: https://github.com/Senbyo/meshletmaker
	static void applyMeshletAlgorithmGreedyVerts(MeshletIn& in, MeshletIntermediate& temp, MeshletOut& out);
	// go through meshlets and create the buffers needed on GPU side
	static void fillMeshletOutputBuffers(MeshletIn& in, MeshletOut& out);
	static void generateTrianglesAndNeighbours(MeshletIn& in, MeshletIntermediate& temp);
	static void sort(MeshletIn& in, MeshletIntermediate& temp);
	static void logVertex(PBRShader::Vertex& v);
	static void logVertexIndex(PBRShader::Vertex& v, std::vector<PBRShader::Vertex>& vertices);
	static void logTriangleFromGlTF(int num, MeshInfo* mesh);
	static void logTriangleFromMeshlets(int num, MeshInfo* mesh);
	static void logTriangleFromMeshletBuffers(int num, MeshInfo* mesh);
	static void markVertexOfTriangle(int num, MeshInfo* mesh);
	void checkVertexDuplication(std::string id);
	static void prepareMeshletVertexIndices(MeshletIntermediate& temp);
};

// 
class WorldObject {
public:
	WorldObject();
	virtual ~WorldObject();
	glm::vec3& pos();
	glm::vec3& rot();
    glm::vec3& scale();
	MeshInfo* mesh = nullptr;
	float alpha = 1.0f;
	bool disableSkinning = false; // set to true for animated object to use as fixed mesh
	bool isNonKeyframeAnimated = false; // signal that poses are not interpoalted by Path, but computed outside and set in update()
	int visible; // visible in current view frustrum: 0 == no, 1 == intersection, 2 == completely visible
	void setAction(std::string name);
	// return current bounding box by scanning all vertices, used for bone animated objects
	// if maximise is true bounding box may increase with each call, depending on current animation
	// (used to get max bounding box for animated objects)
	void calculateBoundingBox(BoundingBox& box, bool maximise = false);
	// override the bounding box from mesh data, useful for bone animated objects
	void forceBoundingBox(BoundingBox box);
	// get bounding box either from mesh data, or the one overridden by forceBoundingBox()
	void getBoundingBox(BoundingBox& box);
	// calculate bounding box in world coords of current object location/rotation/scale 
	void calculateBoundingBoxWorld(glm::mat4 modelToWorld);
	// calculate bounding box and add to line list. bounding box is in world coords of current object location/rotation/scale 
	void drawBoundingBox(std::vector<LineDef>& boxes, glm::mat4 modelToWorld, glm::vec4 color = Colors::Silver);
	// calc distance. Currently refers to object origin which might be incorrect in some cases, TODO implement distance to bounding box center
    float distanceTo(glm::vec3 pos);
    bool isLineIntersectingBoundingBox(const glm::vec3& lineStart, const glm::vec3& lineEnd);
	glm::vec3 objectStartPos;
	// 3d sound 
	bool stopped; // a running cue may temporarily stopped
	bool playing; // true == current cue is actually running
	SoundDef* soundDef = nullptr;
	int maxListeningDistance; // disable sound if farther away than this
	// no longer used - see Sound.h //int soundListIndex;  // index into audibleWorldObjects, used to get the 3d sound settings for this object, see Sound.h

	bool enableDebugGraphics;
    bool enabled = true;
	UINT objectNum; // must be unique for all objects
    void addVerticesToLineList(std::vector<LineDef>& lines, glm::vec3 offset, float sizeFactor = 1.0f);
    int userGroupId = 0; // user defined group id, used to group objects for specific purposes and easily differentiate them in user code
private:
	glm::vec3 _pos;
	glm::vec3 _rot;
    glm::vec3 _scale = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 bboxVertexMin = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	glm::vec3 bboxVertexMax = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	// 8 corners of bounding box in world coords:
	// low y in first 4 corners, in clockwise order
	BoundingBoxCorners boundingBoxCorners;
};

// WorldObject Store is for displaying loaded objects from MeshStore within the game world
// possibly more than once for the same Mesh
class WorldObjectStore {
public:
	WorldObjectStore(MeshStore* store) {
		meshStore = store;
	}
	// objects
	// add loaded object to scene
	// remember returned ptr for single object access
	WorldObject* addObject(std::string groupname, std::string id, glm::vec3 pos);
	// obbject groups: give fast access to specific objects (e.g. all worm NPCs).
	// an int defining the group type may be given, it will be stored in each object of that group.
	// allows for grouping obejcts more easily in application code
	void createGroup(std::string groupname, int groupId = 0);
	const std::vector<std::unique_ptr<WorldObject>>* getGroup(std::string groupname);
	// get sorted object list (sorted by type)
	// meshes are only resorted if one was added in the meantime
	const std::vector<WorldObject*>& getSortedList();
private:
	std::unordered_map<std::string, std::vector<std::unique_ptr<WorldObject>>> groups;
	StringIntMap groupNames;
	void addObjectPrivate(WorldObject* w, std::string id, glm::vec3 pos, int userGroupId);
	MeshStore *meshStore;
	std::vector<WorldObject*> sortedList;
    UINT numObjects = 0; // count all objects
};