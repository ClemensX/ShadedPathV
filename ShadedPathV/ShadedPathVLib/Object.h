class Util;
struct ObjectInfo
{
	string id;
	string filename;
	bool available = false; // true if this object is ready for use in shader code

	// gltf data: valid after object load, should be cleared after upload
	vector<vec3> vertices;
	vector<uint32_t> indices;

	// GPU data:
	VkBuffer vertexBuffer = nullptr;
	VkDeviceMemory vertexBufferMemory = nullptr;
	VkBuffer indexBuffer = nullptr;
	VkDeviceMemory indexBufferMemory = nullptr;
};
typedef ObjectInfo* ObjectID;

// Object Store:
class ObjectStore {
public:
	// init object store
	void init(ShadedPathEngine* engine);
	~ObjectStore();
	// load object wireframe and add to Line vector
	void loadObjectWireframe(string filename, string id, vector<LineDef>& lines);
	// load object, objects are referenced via id string
	void loadObject(string filename, string id);
	// get sorted object list (sorted by type)
	// objects are only resorted if one was added in the meantime
	const vector<ObjectInfo*> &getSortedList();
	// upload single model to GPU
	void uploadObject(ObjectInfo* obj);

	ObjectInfo* getObject(string id);
private:
	ObjectInfo* loadObjectFile(string filename, string id, vector<byte> &fileBuffer);
	unordered_map<string, ObjectInfo> objects;
	ShadedPathEngine* engine = nullptr;
	Util* util;
	vector<ObjectInfo*> sortedList;
};