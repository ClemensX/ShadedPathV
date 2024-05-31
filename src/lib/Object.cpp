#include "mainheader.h"

using namespace std;
using namespace glm;


void MeshStore::init(ShadedPathEngine* engine) {
	this->engine = engine;
	gltf.init(engine);
}

// simple id, only letters, numbers and underscore
static bool ok_meshid_short_format(const std::string& nom) {
	static const std::regex re{ "^[a-zA-Z][a-zA-Z0-9_]+$" };
	return std::regex_match(nom, re);
}

// long id in format name.number
static bool ok_meshid_long_format(const std::string& nom) {
	static const std::regex re{ "^[a-zA-Z][a-zA-Z0-9_]+([.][0-9]+)?$" };
	return std::regex_match(nom, re);
}

MeshInfo* MeshStore::getMesh(string id)
{
	if (meshes.find(id) == meshes.end()) {
		return nullptr;
	}
	MeshInfo*ret = &meshes[id];
	// simple validity check for now:
	if (ret->id.size() > 0) {
		// if there is no id the texture could not be loaded (wrong filename?)
		ret->available = true;
	}
	else {
		ret->available = false;
	}
	return ret;
}

MeshInfo* MeshStore::initMeshInfo(MeshCollection* coll, std::string id)
{
	MeshInfo initialObject;  // only used to initialize struct in texture store - do not access this after assignment to store

	// add MeshInfo to global and collecion mesh lists
	initialObject.id = id;
	initialObject.collection = coll;
	meshes[id] = initialObject;
	MeshInfo* mi = &meshes[id];
	coll->meshInfos.push_back(mi);
	return mi;
}

MeshCollection* MeshStore::loadMeshFile(string filename, string id, vector<byte>& fileBuffer)
{
	if (getMesh(id) != nullptr) {
		Error("Cannot store 2 meshes with same ID in MeshStore.");
	}
	if (ok_meshid_short_format(id) == false) {
		stringstream s;
		s << "WorldObjectStore: wrong id format " << id << endl;
		Error(s.str());
	}
	// create MeshCollection and one MexhInfo: we have at least one mesh per gltf file
	MeshCollection initialCollection;  // only used to initialize struct in texture store - do not access this after assignment to store
	initialCollection.id = id;
	meshCollections.push_back(initialCollection);
	MeshCollection* collection = &meshCollections.back();

	MeshInfo* mi = initMeshInfo(collection, id);

	// find texture file, look in pak file first:
	PakEntry* pakFileEntry = nullptr;
	pakFileEntry = engine->files.findFileInPak(filename.c_str());
	// try file system if not found in pak:
	collection->filename = filename;
	string binFile;
	if (pakFileEntry == nullptr) {
		binFile = engine->files.findFile(filename.c_str(), FileCategory::MESH);
		collection->filename = binFile;
		//initialTexture.filename = binFile;
		engine->files.readFile(collection->filename.c_str(), fileBuffer, FileCategory::MESH);
	}
	else {
		engine->files.readFile(pakFileEntry, fileBuffer, FileCategory::MESH);
	}
	return collection;
}

void MeshStore::loadMeshWireframe(string filename, string id, vector<LineDef> &lines)
{
	vector<byte> file_buffer;
	MeshCollection* coll = loadMeshFile(filename, id, file_buffer);
	MeshInfo* obj = coll->meshInfos[0];
	string fileAndPath = coll->filename;
	vector<PBRShader::Vertex> vertices;
	vector<uint32_t> indexBuffer;
	gltf.loadVertices((const unsigned char*)file_buffer.data(), (int)file_buffer.size(), obj, vertices, indexBuffer, fileAndPath);
	if (vertices.size() > 0) {
		for (uint32_t i = 0; i < indexBuffer.size(); i += 3) {
			// triangle i --> i+1 --> i+2
			LineDef l;
			l.color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			auto& p0 = vertices[indexBuffer[i]];
			auto& p1 = vertices[indexBuffer[i+1]];
			auto& p2 = vertices[indexBuffer[i+2]];
			l.start = p0.pos;
			l.end = p1.pos;
			lines.push_back(l);
			l.start = p1.pos;
			l.end = p2.pos;
			lines.push_back(l);
			l.start = p2.pos;
			l.end = p0.pos;
			lines.push_back(l);
		}
	}
	obj->available = true;
}

void MeshStore::loadMesh(string filename, string id)
{
	vector<byte> file_buffer;
	MeshCollection* coll = loadMeshFile(filename, id, file_buffer);
	string fileAndPath = coll->filename;
	gltf.load((const unsigned char*)file_buffer.data(), (int)file_buffer.size(), coll, fileAndPath);
	coll->available = true;
}

void MeshStore::uploadObject(MeshInfo* obj)
{
	assert(obj->vertices.size() > 0);
	assert(obj->indices.size() > 0);

	// upload vec3 vertex buffer:
	size_t vertexBufferSize = obj->vertices.size() * sizeof(PBRShader::Vertex);
	size_t indexBufferSize = obj->indices.size() * sizeof(obj->indices[0]);
	engine->global.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize, obj->vertices.data(), obj->vertexBuffer, obj->vertexBufferMemory, "GLTF object vertex buffer");
	engine->global.uploadBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBufferSize, obj->indices.data(), obj->indexBuffer, obj->indexBufferMemory, "GLTF object index buffer");
}

const vector<MeshInfo*> &MeshStore::getSortedList()
{
	if (sortedList.size() == meshes.size()) {
		return sortedList;
	}
	// create list, TODO sorting
	sortedList.clear();
	sortedList.reserve(meshes.size());
	for (auto& kv : meshes) {
		sortedList.push_back(&kv.second);
	}
	return sortedList;
}

MeshStore::~MeshStore()
{
	for (auto& mapobj : meshes) {
		if (mapobj.second.available) {
			auto& obj = mapobj.second;
			vkDestroyBuffer(engine->global.device, obj.vertexBuffer, nullptr);
			vkFreeMemory(engine->global.device, obj.vertexBufferMemory, nullptr);
			vkDestroyBuffer(engine->global.device, obj.indexBuffer, nullptr);
			vkFreeMemory(engine->global.device, obj.indexBufferMemory, nullptr);
		}
	}

}

WorldObject::WorldObject() {
	scale = 1.0f;
	drawBoundingBox = false;
	drawNormals = false;
	objectNum = count++;
}

WorldObject::~WorldObject() {
}

vec3& WorldObject::pos() {
	return _pos;
}

vec3& WorldObject::rot() {
	return _rot;
}

void WorldObjectStore::createGroup(string groupname) {
	if (groups.count(groupname) > 0) return;  // do not recreate groups
											  //vector<WorldObject> *newGroup = groups[groupname];
	const auto& newGroup = groups[groupname];
	Log(" ---groups size " << groups.size() << endl);
	Log(" ---newGroup vecor size " << newGroup.size() << endl);
}

const vector<unique_ptr<WorldObject>>* WorldObjectStore::getGroup(string groupname) {
	if (groups.count(groupname) == 0) return nullptr;
	return &groups[groupname];
}

WorldObject* WorldObjectStore::addObject(string groupname, string id, vec3 pos) {
	if (groups.count(groupname) == 0) {
		stringstream s;
		s << "WorldObjectStore: trying to add object to non-existing group " << groupname << endl;
		Error(s.str());
	}
	auto& grp = groups[groupname];
	grp.push_back(unique_ptr<WorldObject>(new WorldObject()));
	WorldObject* w = grp[grp.size() - 1].get();
	addObjectPrivate(w, id, pos);
	return w;
}

void WorldObjectStore::addObject(WorldObject& w, string id, vec3 pos) {
	addObjectPrivate(&w, id, pos);
}

void WorldObjectStore::addObjectPrivate(WorldObject* w, string id, vec3 pos) {
	if (ok_meshid_long_format(id) == false) {
		stringstream s;
		s << "WorldObjectStore: trying to add object with wrong id format " << id << endl;
		Error(s.str());
	} else {
		// we need to check for name.0 format and rename to just 'name':
		if (id.ends_with(".0")) {
			id = id.substr(0, id.length() - 2);
		}
	}
	MeshInfo* mesh = meshStore->getMesh(id);
	if (mesh == nullptr) {
		stringstream s;
		s << "WorldObjectStore: Trying to load non-existing object " << id << endl;
		Error(s.str());
	}
	w->pos() = pos;
	w->objectStartPos = pos;
	w->mesh = mesh;
	w->alpha = 1.0f;
	numObjects++;
}

const vector<WorldObject*>& WorldObjectStore::getSortedList()
{
	if (sortedList.size() == numObjects) {
		return sortedList;
	}
	// create list, todo: sorting
	sortedList.clear();
	sortedList.reserve(numObjects);
	for (auto& gm : groups) {
		auto &grp = gm.second;
		for (auto &o : grp) {
			sortedList.push_back(o.get());
		}
	}
	return sortedList;
}

atomic<UINT> WorldObject::count;
