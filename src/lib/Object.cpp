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
	size_t meshletIndexBufferSize = obj->meshletVertexIndices.size() * sizeof(obj->meshletVertexIndices[0]);
	engine->globalRendering.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize, obj->vertices.data(), obj->vertexBuffer, obj->vertexBufferMemory, "GLTF object vertex buffer");
	if (obj->meshletVertexIndices.size() > 0)
		engine->globalRendering.uploadBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, meshletIndexBufferSize, obj->meshletVertexIndices.data(), obj->indexBuffer, obj->indexBufferMemory, "GLTF object index buffer");
	else 
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
	enableDebugGraphics = false;
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

void MeshInfo::getBoundingBox(BoundingBox& box)
{
	if (boundingBoxAlreadySet) {
		box = boundingBox;
		return;
	}
	// iterate through vertices and find min/max:
	for (auto& v : this->vertices) {
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

void WorldObject::getBoundingBox(BoundingBox& box)
{
    return mesh->getBoundingBox(box);
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

void MeshStore::debugGraphics(WorldObject* obj, FrameResources& fr, glm::mat4 modelToWorld, vec4 color, float normalLineLength)
{
	// get access to line shader
	auto& lineShader = engine->shaders.lineShader;
	if (!lineShader.enabled) return;
    if (!obj->enableDebugGraphics) return;

	vector<LineDef> addLines;
	obj->drawBoundingBox(addLines, modelToWorld, color);

	// add normals:
	for (auto& v : obj->mesh->vertices) {
		LineDef l;
		l.color = color;
		l.start = vec3(modelToWorld * vec4(v.pos, 1.0f));
		l.end = l.start + v.normal * normalLineLength;
		addLines.push_back(l);
    }
	
	//corner = vec3(modelToWorld * vec4(corner, 1.0f));


	lineShader.prepareAddLines(fr);
	lineShader.addOneTime(addLines, fr);
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

bool CompareTriangles(const Meshlet::MeshletTriangle* t1, const Meshlet::MeshletTriangle* t2, const int idx) {
	return (t1->centroid[idx] < t2->centroid[idx]);
}

bool compareVerts(const Meshlet::MeshletVertInfo* v1, const Meshlet::MeshletVertInfo* v2, const PBRShader::Vertex* vertexBuffer, const int idx) {
	return (vertexBuffer[v1->index].pos[idx] < vertexBuffer[v2->index].pos[idx]);
}

void MeshStore::calculateMeshlets(std::string id, uint32_t vertexLimit, uint32_t primitiveLimit)
{
#   if defined(DEBUG)
    checkVertexNormalConsistency(id);
#   endif
	assert(primitiveLimit <= 255); // we need one more primitive for adding the 'rest'
	assert(vertexLimit <= 256);

	MeshInfo* mesh = getMesh(id);
	//mesh->
	// min	[-0.040992 -0.046309 -0.053326]	glm::vec<3,float,0>
	// max	[0.040992 0.067943 0.132763]	glm::vec<3,float,0>
    BoundingBox box;
    mesh->getBoundingBox(box);
    Log("bounding box min: " << box.min.x << " " << box.min.y << " " << box.min.z << endl);
    Log("bounding box max: " << box.max.x << " " << box.max.y << " " << box.max.z << endl);

    // make vertices and indices unique:
    unordered_map<PBRShader::Vertex, uint32_t> uniqueVertexMap; // map vertex to index
	vector<PBRShader::Vertex> uniqueVertices;
	vector<uint32_t> uniqueIndices;

    uint32_t uniqueIndex = 0;
	uint32_t indexNum = 0; // count iterations over indices
	for (auto& index : mesh->indices) {
		auto& v = mesh->vertices[index];
		if (indexNum == 108620) {
			auto& p = v.pos;
		}
		auto lookup = uniqueVertexMap.find(v);
		if (lookup != uniqueVertexMap.end()) {
			// vertex already exists, reuse index
			uniqueIndex = lookup->second;
			auto& p = v.pos;
			//std::cout << "indexNum " << indexNum << " v index " << uniqueIndex << " reused vertex " << p.x << " " << p.y << " " << p.z << std::endl;
		}
		else {
			// new vertex, add to map
			uint32_t newIndex = uniqueVertices.size();
			uniqueVertices.push_back(v);
			if (uniqueVertices.size() == 18640) {
				//std::cout << "Too many vertices, stopping at " << uniqueVertices.size() << std::endl;
			}
			uniqueVertexMap[v] = newIndex;
			uniqueIndex = newIndex;
		}
        uniqueIndices.push_back(uniqueIndex);
		indexNum++;
    }

	// initiate meshlets: build MeshletTriangles and MeshletVertInfos from global indices

	std::unordered_map<uint32_t, Meshlet::MeshletVertInfo> indexVertexMap;
    std::vector<Meshlet::MeshletTriangle*> triangles; // pointers to triangles, used in algorithms
    std::vector<Meshlet::MeshletTriangle> trianglesVector; // base storage for triangles
	//std::vector<Meshlet::MeshletVertInfo> vertices;

	// Generate mesh structure
	triangles.resize(uniqueIndices.size() / 3);
	trianglesVector.resize(triangles.size());
	int unique = 0;
	int reused = 0;
	for (uint32_t i = 0; i < uniqueIndices.size() / 3; i++) {
		Meshlet::MeshletTriangle* t = &trianglesVector[i];
		t->id = i;
		triangles[i] = t;
		for (uint32_t j = 0; j < 3; j++) {
			auto lookup = indexVertexMap.find(uniqueIndices[i * 3 + j]);
			if (lookup != indexVertexMap.end()) {
				// vertex already exists, reuse it
				lookup->second.neighbours.push_back(t);
				lookup->second.degree++;
				t->vertices.push_back(&lookup->second);
				reused++;
                //std::cout << "reused vertex " << lookup->second.index << std::endl;
			}
			else {
                Meshlet::MeshletVertInfo v;
                v.index = uniqueIndices[i * 3 + j];
                v.degree = 1;
                v.neighbours.push_back(t);
                indexVertexMap[v.index] = v;
                t->vertices.push_back(&indexVertexMap[v.index]);
				unique++;
			}

		}
	}
	Log("Mesh structure initialised" << std::endl);
	Log(unique << " vertices added " << reused << " reused " << (unique + reused) << " total" << std::endl);

	// Connect vertices
	uint32_t found;
	Meshlet::MeshletTriangle* t;
	Meshlet::MeshletTriangle* c;
	Meshlet::MeshletVertInfo* v;
	Meshlet::MeshletVertInfo* p;
	for (uint32_t i = 0; i < triangles.size(); ++i) { // For each triangle
		t = triangles[i];
		// Find adjacent triangles
		found = 0;
		for (uint32_t j = 0; j < 3; ++j) { // For each vertex of each triangle
			v = t->vertices[j];
			for (uint32_t k = 0; k < v->neighbours.size(); ++k) { // For each triangle containing each vertex of each triangle
				c = v->neighbours[k];
				if (c->id == t->id) continue; // You are yourself a neighbour of your neighbours
				for (uint32_t l = 0; l < 3; ++l) { // For each vertex of each triangle containing ...
					p = c->vertices[l];
					if (p->index == t->vertices[(j + 1) % 3]->index) {
						found++;
						t->neighbours.push_back(c);
						break;
					}
				}
			}

		}
		//if (found != 3) {
		//	std::cout << "Failed to find 3 adjacent triangles found " << found << " idx: " << t->id << std::endl;
		//}
	}

    // replace vertexBuffer with unique vertices:
    mesh->vertices.clear();
    mesh->vertices.reserve(uniqueVertices.size());
	for (auto& v : uniqueVertices) {
		mesh->vertices.push_back(v);
    }


    // calculate cetroids and bounding boxes for triangles
	std::vector<Meshlet> meshlets;
	std::vector<Meshlet::MeshletVertInfo*> vertsVector;
    auto& vertexBuffer = mesh->vertices; // use the original vertex buffer for sorting
	glm::vec3 min{ FLT_MAX };
	glm::vec3 max{ FLT_MIN };
	for (Meshlet::MeshletTriangle& tri : trianglesVector) {
		//glm::vec3 v1 = vertexBuffer[tri->vertices[0]->index].pos;
		//glm::vec3 v2 = vertexBuffer[tri->vertices[1]->index].pos;
		//glm::vec3 v3 = vertexBuffer[tri->vertices[2]->index].pos;

		min = glm::min(min, vertexBuffer[tri.vertices[0]->index].pos);
		min = glm::min(min, vertexBuffer[tri.vertices[1]->index].pos);
		min = glm::min(min, vertexBuffer[tri.vertices[2]->index].pos);
		max = glm::max(max, vertexBuffer[tri.vertices[0]->index].pos);
		max = glm::max(max, vertexBuffer[tri.vertices[1]->index].pos);
		max = glm::max(max, vertexBuffer[tri.vertices[2]->index].pos);

		//min = glm::min(min, v1);
		//min = glm::min(min, v2);
		//min = glm::min(min, v3);
		//max = glm::max(max, v1);
		//max = glm::max(max, v2);
		//max = glm::max(max, v3);

		glm::vec3 centroid = (vertexBuffer[tri.vertices[0]->index].pos + vertexBuffer[tri.vertices[1]->index].pos + vertexBuffer[tri.vertices[2]->index].pos) / 3.0f;
		//glm::vec3 centroid = (v1 + v2 + v3) / 3.0f;
		tri.centroid[0] = centroid.x;
		tri.centroid[1] = centroid.y;
		tri.centroid[2] = centroid.z;
	}

	// use the same axis info to sort vertices
	glm::vec3 axis = glm::abs(max - min);


	vertsVector.reserve(indexVertexMap.size());
	for (int i = 0; i < indexVertexMap.size(); ++i) {
		vertsVector.push_back(&indexVertexMap[i]);
	}

	if (axis.x > axis.y && axis.x > axis.z) {
        sortByPosAxis(vertsVector, vertexBuffer, Axis::X);
        sortTrianglesByPosAxis(triangles, vertexBuffer, Axis::X);
		//std::sort(vertsVector.begin(), vertsVector.end(), std::bind(compareVerts, std::placeholders::_1, std::placeholders::_2, vertexBuffer, 0));
		//std::sort(triangles.begin(), triangles.end(), std::bind(CompareTriangles, std::placeholders::_1, std::placeholders::_2, 0));
		std::cout << "x sorted" << std::endl;
	}
	else if (axis.y > axis.z && axis.y > axis.x) {
		sortByPosAxis(vertsVector, vertexBuffer, Axis::Y);
		sortTrianglesByPosAxis(triangles, vertexBuffer, Axis::Y);
		//std::sort(vertsVector.begin(), vertsVector.end(), std::bind(compareVerts, std::placeholders::_1, std::placeholders::_2, vertexBuffer, 1));
		//std::sort(triangles.begin(), triangles.end(), std::bind(CompareTriangles, std::placeholders::_1, std::placeholders::_2, 1));
		std::cout << "y sorted" << std::endl;
	}
	else {
		sortByPosAxis(vertsVector, vertexBuffer, Axis::Z);
		sortTrianglesByPosAxis(triangles, vertexBuffer, Axis::Z);
		//std::sort(vertsVector.begin(), vertsVector.end(), std::bind(compareVerts, std::placeholders::_1, std::placeholders::_2, vertexBuffer, 2));
		//std::sort(triangles.begin(), triangles.end(), std::bind(CompareTriangles, std::placeholders::_1, std::placeholders::_2, 2));
		std::cout << "z sorted" << std::endl;
	}
	static auto col = engine->util.generateColorPalette256();
	mesh->meshletVertexIndices.reserve(vertsVector.size());
	int vertCount = 0, colIndex = 0;
	uint32_t vIndexMax = 0;
    uint32_t vIndexMin = UINT32_MAX;
	for (auto& v : vertsVector) {
		vertCount++;
		//std::cout << "vertex " << v->index << " pos: " << vertexBuffer[v->index].pos.x << " " << vertexBuffer[v->index].pos.y << " " << vertexBuffer[v->index].pos.z << std::endl;
        if (vertCount % 1000 == 0) {
			colIndex++;
        }
        mesh->meshletVertexIndices.push_back(v->index);
        vertexBuffer[v->index].color = col[colIndex % 256]; // assign color from palette
		//vertexBuffer[v->index].color = vec4(1.0f, 0.0f, 0.0f, 1.0f); // mark vertices as meshlet vertices
		if (v->index > vIndexMax) {
			vIndexMax = v->index;
        }
		if (v->index < vIndexMin) {
			vIndexMin = v->index;
		}
	}
    Log("Meshlet vertex indices min: " << vIndexMin << " max: " << vIndexMax << " out of total vertices from file: " << vertexBuffer.size() << endl);
	//return;
	mesh->meshletVertexIndices.clear();
	mesh->meshletVertexIndices.reserve(triangles.size()*3);
    int triCount = 0;
    int triColIndex = 0;
	for (auto& tri : trianglesVector) {
        triCount++;
        //Log("Triangle " << tri.id << " centroid: " << tri.centroid[0] << " " << tri.centroid[1] << " " << tri.centroid[2] << endl);
        assert(tri.vertices.size() == 3);
		auto idx = tri.vertices[0]->index;
        auto& v0 = vertexBuffer[idx];
		mesh->meshletVertexIndices.push_back(idx);
		idx = tri.vertices[1]->index;
		auto& v1 = vertexBuffer[idx];
		mesh->meshletVertexIndices.push_back(idx);
		idx = tri.vertices[2]->index;
		auto& v2 = vertexBuffer[idx];
		mesh->meshletVertexIndices.push_back(idx);
		if (triCount % 1000 == 0) {
			//triColIndex++;
        }
        auto color = col[triColIndex % 256]; // assign color from palette
        v0.color = color;
        v1.color = color;
        v2.color = color;
    }
	Meshlet::applyMeshletAlgorithmGreedyVerts(
		indexVertexMap, vertsVector, triangles, meshlets, vertexBuffer, primitiveLimit, vertexLimit
    );

	// color the meshlets:
    int meshletCount = 0;
	for (auto& m : meshlets) {
		auto color = col[meshletCount % 256]; // assign color from palette
		meshletCount++;
		for (auto& v : m.verticesIndices) {
			vertexBuffer[v].color = color; // assign color to vertices in meshlet
		}
    }
}

void Meshlet::applyMeshletAlgorithmGreedyVerts(
	std::unordered_map<uint32_t, Meshlet::MeshletVertInfo>& indexVertexMap, // 117008
	std::vector<Meshlet::MeshletVertInfo*>& vertsVector, // 117008
	std::vector<Meshlet::MeshletTriangle*>& triangles, // 231256
	std::vector<Meshlet>& meshlets, // 0
	const std::vector<PBRShader::Vertex>& vertexBuffer, // 117008 vertices
	uint32_t primitiveLimit, uint32_t vertexLimit // 125, 64
)
{
    std::queue<Meshlet::MeshletVertInfo*> priorityQueue;
	std::unordered_map<uint32_t, unsigned char> used;
    Meshlet cache;
	for (uint32_t i = 0; i < vertsVector.size(); i++) {
		MeshletVertInfo* vert = vertsVector[i];
		if (used.find(vert->index) != used.end()) continue;
		// reset
        priorityQueue.push(vert);

		// add triangles to cache until it is full
		while (!priorityQueue.empty()) {
			// pop current triangle
			MeshletVertInfo* vert = priorityQueue.front();

			for (MeshletTriangle* tri : vert->neighbours) {
				if (tri->flag == 1) continue; // already used

				// get all vertices of current triangle
				uint32_t candidateIndices[3];
				for (uint32_t j = 0; j < 3; j++) {
					uint32_t idx = tri->vertices[j]->index;
					candidateIndices[j] = idx;
					if (used.find(idx) == used.end()) priorityQueue.push(tri->vertices[j]); // add to queue if not used
				}
				// break if cache is full
				if (cache.cannotInsert(candidateIndices, vertexLimit, primitiveLimit)) {
					// we run out of verts but could push prims more so we do a pass of prims here to see if we can maximize 
					// so we run through all triangles to see if the meshlet already has the required verts
					// we try to do this in a dum way to test if it is worth it
					for (int v = 0; v < cache.verticesIndices.size(); ++v) {
						for (MeshletTriangle* tri : indexVertexMap[cache.verticesIndices[v]].neighbours) {
							if (tri->flag == 1) continue;

							uint32_t candidateIndices[3];
							for (uint32_t j = 0; j < 3; ++j) {
								uint32_t idx = tri->vertices[j]->index;
								candidateIndices[j] = idx;
								if (used.find(idx) == used.end()) priorityQueue.push(tri->vertices[j]);
							}

							if (!cache.cannotInsert(candidateIndices, vertexLimit, primitiveLimit)) {
								cache.insert(candidateIndices/*, vertexBuffer*/);

								tri->flag = 1;
							}
						}
					}
					meshlets.push_back(cache);

					// reset cache and empty priority queue
					priorityQueue = {};
					priorityQueue.push(vert); // re-add the current vertex to the queue
					cache.reset();
					continue;
					// start over again but from the fringe of the current cluster
				}

				cache.insert(candidateIndices/*, vertexBuffer*/);

				// if triangle is inserted set flag to used
				tri->flag = 1;
			}

			// pop vertex if we make it through all its neighbors
			priorityQueue.pop();
			used[vert->index] = 1; // mark vertex as used
		}; // end while
	}
    // add remaining triangles to a meshlets
	if (!cache.empty()) {
		meshlets.push_back(cache);
		cache.reset();

	}
}

void MeshStore::checkVertexNormalConsistency(std::string id)
{
	MeshInfo* mesh = getMesh(id);

	// copy vertex vector
    vector<PBRShader::Vertex> vsort = mesh->vertices;
    // sort vertices by position
	sortByPos(vsort);
    // count normals per vertex:
    unordered_map<uint32_t, vec3> vertexNormals; // map vertex index to normal
	int count = 1;
    long separateVertexPos = 1; // count vertices with different positions
    long totalVertices = 0;
    int maxVertexResuse = 1;
	for (size_t i = 1; i < vsort.size(); i++) {
		auto& v = vsort[i];
		auto& vlast = vsort[i-1];
        totalVertices++;
		if (v.pos == vlast.pos) {
			// vertex is duplicate, count
			count++;
		}
		else {
            separateVertexPos++;
            if (count > maxVertexResuse) {
                maxVertexResuse = count;
            }
            count = 1; // reset count
		}
    }
	float ratio = (float)totalVertices / (float)separateVertexPos;
	Log("MeshStore ratio " << ratio << " total vertices: " << totalVertices << " separate vertices: " << separateVertexPos << " max single vertex reuse: " << maxVertexResuse << endl);
	if (ratio > VERTEX_REUSE_THRESHOLD) {
		Log("WARNING: MeshStore: Mesh " << id << " has a high ratio of total vertices to separate vertices, consider optimizing the mesh." << endl);
	}
	else {
		Log("MeshStore: Mesh " << id << " has a good ratio of total vertices to separate vertices." << endl);
    }
}
