class Util;
struct SoundDef;

// all meshes loaded from one gltf file. Textures are maintained here, meshes are contained
struct MeshCollection
{
	std::string id;
	std::string filename;
	bool available = false; // true if this object is ready for use in shader code
	std::vector<ktxTexture*> textureParseInfo;
	std::vector<::TextureInfo*> textureInfos; // we check for max size in 
	std::vector<MeshInfo*> meshInfos;
};

// Describe a single loaded mesh. mesh IDs are unique, several Objects may be instantiated backed by the same mesh
struct MeshInfo
{
	std::string id;
	bool available = false; // true if this object is ready for use in shader code

	// gltf data: valid after object load, should be cleared after upload
	std::vector<PBRShader::Vertex> vertices;
	std::vector<uint32_t> indices;
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
	//VkDescriptorSet descriptorSet = nullptr;

	// gltf link, only valid during gltf parsing!
	void* gltfMesh = nullptr;

	// link back to collection
	MeshCollection* collection = nullptr;
};
typedef MeshInfo* ObjectID;

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
	void loadMesh(std::string filename, std::string id);
	// get sorted object list (sorted by type)
	// meshes are only resorted if one was added in the meantime
	const std::vector<MeshInfo*> &getSortedList();
	// upload single model to GPU
	void uploadObject(MeshInfo* obj);
	// initialize MeshInfo, also add to collection. id is expected to be in collection format like myid.2
	// myid.0 is a synonym for myid
	MeshInfo* initMeshInfo(MeshCollection* coll, std::string id);

	MeshInfo* getMesh(std::string id);
private:
	MeshCollection* loadMeshFile(std::string filename, std::string id, std::vector<std::byte> &fileBuffer);
	std::unordered_map<std::string, MeshInfo> meshes;
	std::vector<MeshCollection> meshCollections;
	ShadedPathEngine* engine = nullptr;
	Util* util = nullptr;
	std::vector<MeshInfo*> sortedList;
	glTF gltf;
};


class BoundingBox {
public:
	// initialize min to larges value and vice-versa, simplifies calculations
	glm::vec3 min = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	glm::vec3 max = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
};

// 
class WorldObject {
public:
	static std::atomic<UINT> count; // count all objects
	WorldObject();
	virtual ~WorldObject();
	glm::vec3& pos();
	glm::vec3& rot();
	MeshInfo* mesh = nullptr;
	float alpha = 1.0f;
	bool disableSkinning = false; // set to true for animated object to use as static meshes
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
	glm::vec3 objectStartPos;
	// 3d sound 
	bool stopped; // a running cue may temporarily stopped
	bool playing; // true == current cue is actually running
	SoundDef* soundDef = nullptr;
	int maxListeningDistance; // disable sound if farther away than this
	// no longer used - see Sound.h //int soundListIndex;  // index into audibleWorldObjects, used to get the 3d sound settings for this object, see Sound.h

	float scale;
	bool drawBoundingBox;
	bool drawNormals;
	UINT objectNum; // must be unique for all objects
private:
	glm::vec3 _pos;
	glm::vec3 _rot;
	glm::vec3 bboxVertexMin = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	glm::vec3 bboxVertexMax = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	bool boundingBoxAlreadySet = false;
	BoundingBox boundingBox;
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
	// TODO seems unused - remove
	void addObject(WorldObject& w, std::string id, glm::vec3 pos);
	// obbject groups: give fast access to specific objects (e.g. all worm NPCs)
	void createGroup(std::string groupname);
	const std::vector<std::unique_ptr<WorldObject>>* getGroup(std::string groupname);
	// get sorted object list (sorted by type)
	// meshes are only resorted if one was added in the meantime
	const std::vector<WorldObject*>& getSortedList();
private:
	std::unordered_map<std::string, std::vector<std::unique_ptr<WorldObject>>> groups;
	void addObjectPrivate(WorldObject* w, std::string id, glm::vec3 pos);
	MeshStore *meshStore;
	std::vector<WorldObject*> sortedList;
	UINT numObjects = 0;
};