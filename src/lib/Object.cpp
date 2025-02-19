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
	initialObject.flags = coll->flags;
	meshes[id] = initialObject;
	MeshInfo* mi = &meshes[id];
	coll->meshInfos.push_back(mi);
	return mi;
}

MeshCollection* MeshStore::loadMeshFile(string filename, string id, vector<byte>& fileBuffer, MeshFlagsCollection flags)
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
	collection->flags = flags;
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
	MeshFlagsCollection flags;
	MeshCollection* coll = loadMeshFile(filename, id, file_buffer, flags);
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

void MeshStore::loadMesh(string filename, string id, MeshFlagsCollection flags)
{
	vector<byte> file_buffer;
	MeshCollection* coll = loadMeshFile(filename, id, file_buffer, flags);
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
	engine->globalRendering.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize, obj->vertices.data(), obj->vertexBuffer, obj->vertexBufferMemory, "GLTF object vertex buffer");
	engine->globalRendering.uploadBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBufferSize, obj->indices.data(), obj->indexBuffer, obj->indexBufferMemory, "GLTF object index buffer");
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

// we either have single meshes in meshes or collections in meshCollections, delete all with flag available == true
MeshStore::~MeshStore()
{
	for (auto& coll : meshCollections) {
		if (coll.available) {
            for (auto& obj : coll.meshInfos) {
				vkDestroyBuffer(engine->globalRendering.device, obj->vertexBuffer, nullptr);
				vkFreeMemory(engine->globalRendering.device, obj->vertexBufferMemory, nullptr);
				vkDestroyBuffer(engine->globalRendering.device, obj->indexBuffer, nullptr);
				vkFreeMemory(engine->globalRendering.device, obj->indexBufferMemory, nullptr);
                obj->available = false;
            }
		}
	}
	for (auto& mapobj : meshes) {
		if (mapobj.second.available) {
			auto& obj = mapobj.second;
			vkDestroyBuffer(engine->globalRendering.device, obj.vertexBuffer, nullptr);
			vkFreeMemory(engine->globalRendering.device, obj.vertexBufferMemory, nullptr);
			vkDestroyBuffer(engine->globalRendering.device, obj.indexBuffer, nullptr);
			vkFreeMemory(engine->globalRendering.device, obj.indexBufferMemory, nullptr);
		}
	}
}

WorldObject::WorldObject() {
	drawNormals = false;
}

WorldObject::~WorldObject() {
}

vec3& WorldObject::pos() {
	return _pos;
}

vec3& WorldObject::rot() {
	return _rot;
}

vec3& WorldObject::scale() {
	return _scale;
}

void WorldObject::getBoundingBox(BoundingBox& box)
{
	if (boundingBoxAlreadySet) {
		box = boundingBox;
		return;
	}
	// iterate through vertices and find min/max:
	for (auto & v : mesh->vertices) {
        if (v.pos.x < box.min.x) box.min.x = v.pos.x;
        if (v.pos.y < box.min.y) box.min.y = v.pos.y;
        if (v.pos.z < box.min.z) box.min.z = v.pos.z;
        if (v.pos.x > box.max.x) box.max.x = v.pos.x;
        if (v.pos.y > box.max.y) box.max.y = v.pos.y;
        if (v.pos.z > box.max.z) box.max.z = v.pos.z;
    }
	boundingBox = box;
	boundingBoxAlreadySet = true;
}

void WorldObjectStore::createGroup(string groupname, int groupId) {
	if (groups.count(groupname) > 0) return;  // do not recreate groups
											  //vector<WorldObject> *newGroup = groups[groupname];
	const auto& newGroup = groups[groupname];
    groupNames.add(groupname, groupId);
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
    int grpId = groupNames.get(groupname);
	addObjectPrivate(w, id, pos, grpId);
	return w;
}

void WorldObjectStore::addObjectPrivate(WorldObject* w, string id, vec3 pos, int userGroupId) {
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
    w->userGroupId = userGroupId;
	w->objectNum = numObjects++;
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

void WorldObject::calculateBoundingBoxWorld(glm::mat4 modelToWorld)
{
	BoundingBox box;
	getBoundingBox(box);
    auto& corners = boundingBoxCorners.corners;
	corners[0] = vec3(box.min.x, box.min.y, box.min.z);
	corners[1] = vec3(box.min.x, box.min.y, box.max.z);
	corners[2] = vec3(box.max.x, box.min.y, box.max.z);
	corners[3] = vec3(box.max.x, box.min.y, box.min.z);
	corners[4] = vec3(box.min.x, box.max.y, box.min.z);
	corners[5] = vec3(box.min.x, box.max.y, box.max.z);
	corners[6] = vec3(box.max.x, box.max.y, box.max.z);
	corners[7] = vec3(box.max.x, box.max.y, box.min.z);
	// transform corners to world coords:
	for (vec3& corner : corners) {
		corner = vec3(modelToWorld * vec4(corner, 1.0f));
	}
}

void WorldObject::drawBoundingBox(std::vector<LineDef>& boxes, glm::mat4 modelToWorld, vec4 color)
{
    LineDef l;
    l.color = color;
    calculateBoundingBoxWorld(modelToWorld);
    // draw cube from the eight corners:
	auto& corners = boundingBoxCorners.corners;
	// lower rect:
	l.start = corners[0];
	l.end = corners[1];
	boxes.push_back(l);
	l.start = corners[1];
	l.end = corners[2];
	boxes.push_back(l);
	l.start = corners[2];
	l.end = corners[3];
	boxes.push_back(l);
	l.start = corners[3];
	l.end = corners[0];
	boxes.push_back(l);
    // upper rect:
    l.start = corners[4];
    l.end = corners[5];
    boxes.push_back(l);
    l.start = corners[5];
    l.end = corners[6];
    boxes.push_back(l);
    l.start = corners[6];
    l.end = corners[7];
    boxes.push_back(l);
    l.start = corners[7];
    l.end = corners[4];
    boxes.push_back(l);
    // vertical lines:
    l.start = corners[0];
    l.end = corners[4];
    boxes.push_back(l);
    l.start = corners[1];
    l.end = corners[5];
    boxes.push_back(l);
    l.start = corners[2];
    l.end = corners[6];
    boxes.push_back(l);
    l.start = corners[3];
    l.end = corners[7];
    boxes.push_back(l);
}

void WorldObject::addVerticesToLineList(std::vector<LineDef>& lines, glm::vec3 offset, float sizeFactor)
{
	LineDef l;
	for (long i = 0; i < mesh->indices.size(); i += 3) {
		l.color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
		// triangle is v[0], v[1], v[2]
		auto& v0 = mesh->vertices[mesh->indices[i + 0]];
		auto& v1 = mesh->vertices[mesh->indices[i + 1]];
		auto& v2 = mesh->vertices[mesh->indices[i + 2]];
		l.start = v0.pos * sizeFactor + offset;
		l.end = v1.pos * sizeFactor + offset;
		lines.push_back(l);
		l.start = v1.pos * sizeFactor + offset;
		l.end = v2.pos * sizeFactor + offset;
		lines.push_back(l);
		l.start = v2.pos * sizeFactor + offset;
		l.end = v0.pos * sizeFactor + offset;
		lines.push_back(l);
	}
	//for (long i = 0; i < mesh->vertices.size() - 1; i++) {
	//	l.color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
	//	auto& v1 = mesh->vertices[i];
	//	auto& v2 = mesh->vertices[i+1];
	//	l.start = v1.pos * sizeFactor + offset;
	//	l.end = v2.pos * sizeFactor + offset;
	//	lines.push_back(l);
	//}
}

bool WorldObject::isLineIntersectingBoundingBox(const vec3& lineStart, const vec3& lineEnd) {
    const BoundingBoxCorners& box = boundingBoxCorners;
	vec3 d = lineEnd - lineStart;
	vec3 boxAxes[3] = {
		box.corners[1] - box.corners[0],
		box.corners[3] - box.corners[0],
		box.corners[4] - box.corners[0]
	};

	vec3 axes[6] = {
		normalize(boxAxes[0]),
		normalize(boxAxes[1]),
		normalize(boxAxes[2]),
		normalize(cross(d, boxAxes[0])),
		normalize(cross(d, boxAxes[1])),
		normalize(cross(d, boxAxes[2]))
	};

	for (const vec3& axis : axes) {
		float minBox = numeric_limits<float>::max();
		float maxBox = numeric_limits<float>::lowest();
		for (const vec3& corner : box.corners) {
			float projection = dot(corner, axis);
			minBox = glm::min(minBox, projection);
			maxBox = glm::max(maxBox, projection);
		}

		float minLine = glm::min(dot(lineStart, axis), dot(lineEnd, axis));
		float maxLine = glm::max(dot(lineStart, axis), dot(lineEnd, axis));

		if (maxLine < minBox || minLine > maxBox) {
			return false;
		}
	}

	// debug info if we have an intersection:
    //Log("Intersection detected with line " << lineStart.x << " " << lineStart .y << " " << lineStart.z << " --> " << lineEnd.x << " " << lineEnd.y << " " << lineEnd.z << endl);
    //Log("  axis 0: " << boxAxes[0].x << " " << boxAxes[0].y << " " << boxAxes[0].z << endl);
    //Log("  axis 1: " << boxAxes[1].x << " " << boxAxes[1].y << " " << boxAxes[1].z << endl);
    //Log("  axis 2: " << boxAxes[2].x << " " << boxAxes[2].y << " " << boxAxes[2].z << endl);
	return true;
}

float WorldObject::distanceTo(glm::vec3 pos) {
    return glm::length(pos - _pos);
}
