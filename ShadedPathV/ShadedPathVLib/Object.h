class Util;

// Describe a loaded mesh. mesh IDs are unique, several Objects may be instatiated backed by the same mesh
struct MeshInfo
{
	string id;
	string filename;
	bool available = false; // true if this object is ready for use in shader code

	// gltf data: valid after object load, should be cleared after upload
	vector<PBRShader::Vertex> vertices;
	vector<uint32_t> indices;
	vector<ktxTexture*> textureParseInfo;

	// GPU data:
	VkBuffer vertexBuffer = nullptr;
	VkDeviceMemory vertexBufferMemory = nullptr;
	VkBuffer indexBuffer = nullptr;
	VkDeviceMemory indexBufferMemory = nullptr;
};
typedef MeshInfo* ObjectID;

// Mesh Store to organize objects loaded from gltf files.
class MeshStore {
public:
	// init object store
	void init(ShadedPathEngine* engine);
	~MeshStore();
	// load mesh wireframe and add to Line vector
	void loadMeshWireframe(string filename, string id, vector<LineDef>& lines);
	// load mesh, objects are referenced via id string. Only one mesh for any ID allowed.
	void loadMesh(string filename, string id);
	// get sorted object list (sorted by type)
	// meshes are only resorted if one was added in the meantime
	const vector<MeshInfo*> &getSortedList();
	// upload single model to GPU
	void uploadObject(MeshInfo* obj);

	MeshInfo* getMesh(string id);
private:
	MeshInfo* loadMeshFile(string filename, string id, vector<byte> &fileBuffer);
	unordered_map<string, MeshInfo> meshes;
	ShadedPathEngine* engine = nullptr;
	Util* util;
	vector<MeshInfo*> sortedList;
	glTF gltf;
};


class BoundingBox {
public:
	// initialize min to larges value and vice-versa, simplifies calculations
	vec3 min = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	vec3 max = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
};

// 
class WorldObject {
public:
	static atomic<UINT> count; // count all objects
	WorldObject();
	virtual ~WorldObject();
	vec3& pos();
	vec3& rot();
	MeshInfo* mesh = nullptr;
	float alpha = 1.0f;
	bool disableSkinning = false; // set to true for animated object to use as static meshes
	bool isNonKeyframeAnimated = false; // signal that poses are not interpoalted by Path, but computed outside and set in update()
	int visible; // visible in current view frustrum: 0 == no, 1 == intersection, 2 == completely visible
	void setAction(string name);
	// return current bounding box by scanning all vertices, used for bone animated objects
	// if maximise is true bounding box may increase with each call, depending on current animation
	// (used to get max bounding box for animated objects)
	void calculateBoundingBox(BoundingBox& box, bool maximise = false);
	// override the bounding box from mesh data, useful for bone animated objects
	void forceBoundingBox(BoundingBox box);
	// get bounding box either from mesh data, or the one overridden by forceBoundingBox()
	void getBoundingBox(BoundingBox& box);
	vec3 objectStartPos;
	// 3d sound 
	//int soundListIndex;  // index into audibleWorldObjects, used to get the 3d sound settings for this object, see Sound.h
	bool stopped; // a running cue may temporarily stopped
	bool playing; // true == current cue is actually running
	//SoundDef* soundDef = nullptr;
	int maxListeningDistance; // disable sound if farther away than this
	float scale;
	bool drawBoundingBox;
	bool drawNormals;
	UINT objectNum; // must be unique for all objects
private:
	vec3 _pos;
	vec3 _rot;
	vec3 bboxVertexMin = vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	vec3 bboxVertexMax = vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
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
	void addObject(string groupname, string id, vec3 pos);
	void addObject(WorldObject& w, string id, vec3 pos);
	// obbject groups: give fast access to specific objects (e.g. all worm NPCs)
	void createGroup(string groupname);
	const vector<unique_ptr<WorldObject>>* getGroup(string groupname);
	// get sorted object list (sorted by type)
	// meshes are only resorted if one was added in the meantime
	const vector<WorldObject*>& getSortedList();
private:
	unordered_map<string, vector<unique_ptr<WorldObject>>> groups;
	void addObjectPrivate(WorldObject* w, string id, vec3 pos);
	MeshStore *meshStore;
	vector<WorldObject*> sortedList;
	UINT numObjects = 0;
};