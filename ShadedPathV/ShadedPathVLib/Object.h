class Util;
struct ObjectInfo
{
	string id;
	string filename;
	bool available = false; // true if this object is ready for use in shader code
};
typedef ObjectInfo* ObjectID;

// Object Store:
class ObjectStore {
public:
	// init object store
	void init(ShadedPathEngine* engine);
	~ObjectStore();
	// load object, objects are referenced via id string
	void loadObject(string filename, string id);
	ObjectInfo* getObject(string id);
private:
	unordered_map<string, ObjectInfo> objects;
	ShadedPathEngine* engine = nullptr;
	Util* util;
};