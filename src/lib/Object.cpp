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
	// create MeshCollection and one MeshInfo: we have at least one mesh per gltf file
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
	size_t meshletDescBufferSize = obj->outMeshletDesc.size() * sizeof(PBRShader::PackedMeshletDesc);
	if (meshletDescBufferSize > 0) {
		size_t localIndexBufferSize = obj->outLocalIndexPrimitivesBuffer.size() * sizeof(obj->outLocalIndexPrimitivesBuffer[0]);
		size_t globalIndexBufferSize = obj->outGlobalIndexBuffer.size()         * sizeof(obj->outGlobalIndexBuffer[0]);
		engine->globalRendering.uploadBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, meshletDescBufferSize, obj->outMeshletDesc.data(), obj->meshletDescBuffer, obj->meshletDescBufferMemory, "meshlet desc array buffer");
		engine->globalRendering.uploadBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, localIndexBufferSize, obj->outLocalIndexPrimitivesBuffer.data(), obj->localIndexBuffer, obj->localIndexBufferMemory, "meshlet local index buffer");
		engine->globalRendering.uploadBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, globalIndexBufferSize, obj->outGlobalIndexBuffer.data(), obj->globalIndexBuffer, obj->globalIndexBufferMemory, "meshlet global index buffer");
		engine->globalRendering.uploadBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, vertexBufferSize, obj->vertices.data(), obj->vertexStorageBuffer, obj->vertexStorageBufferMemory, "meshlet vertex buffer");
	}
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
                vkDestroyBuffer(engine->globalRendering.device, obj->meshletDescBuffer, nullptr);
                vkFreeMemory(engine->globalRendering.device, obj->meshletDescBufferMemory, nullptr);
                vkDestroyBuffer(engine->globalRendering.device, obj->globalIndexBuffer, nullptr);
                vkFreeMemory(engine->globalRendering.device, obj->globalIndexBufferMemory, nullptr);
                vkDestroyBuffer(engine->globalRendering.device, obj->localIndexBuffer, nullptr);
                vkFreeMemory(engine->globalRendering.device, obj->localIndexBufferMemory, nullptr);
                vkDestroyBuffer(engine->globalRendering.device, obj->vertexStorageBuffer, nullptr);
                vkFreeMemory(engine->globalRendering.device, obj->vertexStorageBufferMemory, nullptr);
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
			vkDestroyBuffer(engine->globalRendering.device, obj.meshletDescBuffer, nullptr);
			vkFreeMemory(engine->globalRendering.device, obj.meshletDescBufferMemory, nullptr);
			vkDestroyBuffer(engine->globalRendering.device, obj.globalIndexBuffer, nullptr);
			vkFreeMemory(engine->globalRendering.device, obj.globalIndexBufferMemory, nullptr);
			vkDestroyBuffer(engine->globalRendering.device, obj.localIndexBuffer, nullptr);
			vkFreeMemory(engine->globalRendering.device, obj.localIndexBufferMemory, nullptr);
			vkDestroyBuffer(engine->globalRendering.device, obj.vertexStorageBuffer, nullptr);
			vkFreeMemory(engine->globalRendering.device, obj.vertexStorageBufferMemory, nullptr);
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

void MeshStore::applyDebugMeshletColorsToVertices(MeshInfo* mesh)
{
	// color the meshlets:
	int meshletCount = 0;
	static auto col = engine->util.generateColorPalette256();
	for (auto& m : mesh->meshlets) {
		auto color = col[meshletCount % 256]; // assign color from palette
		meshletCount++;
		for (auto& v : m.verticesIndices) {
			mesh->vertices[v].color = color; // assign color to vertices in meshlet
		}
	}
	for (auto& m : mesh->meshletsForMesh.meshlets) {
		auto color = col[meshletCount % 256]; // assign color from palette
		meshletCount++;
		for (auto& v : m.vertices) {
			mesh->vertices[v->globalIndex].color = color; // assign color to vertices in meshlet
		}
	}
}

void MeshStore::applyDebugMeshletColorsToMeshlets(MeshInfo* mesh)
{
	for (auto& m : mesh->meshlets) {
		m.debugColors = true; // mark meshlet as having debug colors
	}
	for (auto& m : mesh->meshletsForMesh.meshlets) {
		m.debugColors = true; // mark meshlet as having debug colors
	}
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

bool CompareTriangles(const MeshletOld::MeshletTriangle* t1, const MeshletOld::MeshletTriangle* t2, const int idx) {
	return (t1->centroid[idx] < t2->centroid[idx]);
}

bool compareVerts(const MeshletOld::MeshletVertInfo* v1, const MeshletOld::MeshletVertInfo* v2, const PBRShader::Vertex* vertexBuffer, const int idx) {
	return (vertexBuffer[v1->index].pos[idx] < vertexBuffer[v2->index].pos[idx]);
}

// Helper function to verify triangle adjacency in meshlet construction
static void verifyAdjacency(const MeshletIn& in, MeshletIntermediate& temp)
{
	// For each triangle, check that its neighbours are valid and that adjacency is consistent
	for (size_t i = 0; i < temp.triangles.size(); ++i) {
		const auto* tri = temp.triangles[i];
		for (const auto* neighbour : tri->neighbours) {
			if (!neighbour) continue;
			// check symmetry
			// Each neighbour should also have this triangle as a neighbour (symmetry)
			bool found = false;
			for (const auto* n2 : neighbour->neighbours) {
				if (n2 == tri) {
					found = true;
					break;
				}
			}
			if (!found) {
				// Log or handle inconsistency if needed
				Log("ERROR: verifyAdjacency: Triangle " << tri->id << " is not reciprocated by neighbour " << neighbour->id << std::endl);
			}
            // Check that the neighbour's vertices include at least one vertex from the triangle
            bool hasVertex = false;
			for (const auto* v : neighbour->vertices) {
				if (v->index == tri->vertices[0]->index || v->index == tri->vertices[1]->index || v->index == tri->vertices[2]->index) {
					hasVertex = true;
					break;
				}
			}
			if (!hasVertex) {
				// Log or handle inconsistency if needed
				Log("ERROR: verifyAdjacency: Neighbour " << neighbour->id << " does not share a vertex with triangle " << tri->id << std::endl);
            }
		}
	}
}

void MeshletsForMesh::verifyGlobalAdjacencyLog(bool runO2BigTest) const
{
    verifyGlobalAdjacency(runO2BigTest, true);
}

bool MeshletsForMesh::verifyGlobalAdjacency(bool runO2BigTest, bool doLog) const
{
    bool ret = true;
	for (int i = 0; i < globalVertices.size(); i++) {
		auto& v = globalVertices[i];
		if (v.usedInTriangle) {
			for (int j = 0; j < v.neighbourTriangles.size(); j++) {
                int neighbourTriangleIndex = v.neighbourTriangles[j];
                // check symmetry: each neighbour triangle should have this vertex in its vertex list
                const GlobalMeshletTriangle& neighbourTriangle = globalTriangles[neighbourTriangleIndex];
                if (!neighbourTriangle.hasVertex(i)) {
                    if (doLog) Log("ERROR: verifyAdjacency: Vertex " << i << " is not reciprocated by neighbour triangle " << neighbourTriangleIndex << std::endl);
                    ret = false;
                }
            }
		}
	}
    // O^2 algorithm to verify adjacency: go through all vertices and check every triangle's vertices for a match
	if (runO2BigTest) {
		for (int i = 0; i < globalVertices.size(); i++) {
			const auto& v = globalVertices[i];
			for (int j = 0; j < globalTriangles.size(); j++) {
				const auto& t = globalTriangles[j];
				if (t.hasVertex(i)) { // v is part of the triangle
					// check if the found triangle is in v's neighbourTriangles
					if (!v.hasNeighbourTriangle(j)) {
						if (doLog) Log("ERROR: verifyAdjacency: Vertex " << i << " is in triangle " << j << " but that triangle is not in its neighbourTriangles." << std::endl);
                        ret = false;
					}
				}
			}
		}
	}
    // check that all vertices are covered by neighbours:
	for (int i = 0; i < globalVertices.size(); i++) {
		const auto& v = globalVertices[i];
		if (v.neighbourTriangles.size() == 0) {
			if (doLog) Log("ERROR: verifyAdjacency: Vertex " << i << " has no neighbour triangles." << std::endl);
            ret = false;
		}
	}
	return ret;
}

void MeshletsForMesh::calculateTrianglesAndNeighbours(MeshletIn2& in)
{
	// Neighbour relation:
    // vertices: all trianges that use this vertex are stored in neighbourTriangles
    // triangles: all triangles that have at least one vertex in common with this triangle are stored in neighbourTriangles
	globalTriangles.resize(in.indices.size() / 3);
	globalVertices.resize(in.vertices.size());
	// create triangle list and
	// add all triangles a vertex belongs to to the vertex neighbours
	for (uint32_t i = 0; i < globalTriangles.size(); i++) {
        GlobalMeshletTriangle* t = &globalTriangles[i];
		for (uint32_t j = 0; j < 3; j++) {
			// fill triangle with vertex indices
            uint32_t vertexIndex = in.indices[i * 3 + j];
			t->indices[j] = vertexIndex;
            // if we find another triangle with the same vertex, we add it's other vertices to the neighbours
            auto& lookup = globalVertices[vertexIndex];
			if (lookup.usedInTriangle == false) {
                // first access to this vertex
                lookup.usedInTriangle = true;
                lookup.globalIndex = vertexIndex;
                lookup.neighbourTriangles.push_back(i);
			} else {
				// vertex already used, add triangle to neighbours
                lookup.neighbourTriangles.push_back(i);
            }
        }
    }
	Log("triangles created: " << globalTriangles.size() << endl);
    // now for the triangles: go through all vertices of each triangle and add all their neighbours to triangle neighbour list
	for (uint32_t tri_index = 0; tri_index < globalTriangles.size(); tri_index++) {
		auto& tri = globalTriangles[tri_index];
		for (uint32_t j = 0; j < 3; j++) {
            auto& v = globalVertices[tri.indices[j]];
			for (auto& neighbourTriangleIndex : v.neighbourTriangles) {
                if (neighbourTriangleIndex == tri_index) continue; // do not add self as neighbour
				if (!tri.hasNeighbourTriangle(neighbourTriangleIndex)) {
					// add triangle to neighbourTriangles of this triangle
					tri.neighbours.push_back(neighbourTriangleIndex);
                }
			}
        }
	}

    verifyGlobalAdjacency();
}

void MeshletsForMesh::applyMeshletAlgorithmSimple(MeshletIn2& in, MeshletOut2& out, int numTrianglesPerMeshlet)
{
	Log("Meshlet algorithm simple started for " << in.vertices.size() << " vertices and " << in.indices.size() << " indices" << std::endl);
	// assert that we have an indices consistent with triangle layout (must be multiple of 3)
	assert(in.indices.size() % 3 == 0);

	if (numTrianglesPerMeshlet == 0) {
        // fill as many triangles as possible into each meshlet
		uint32_t triPos = 0; // current position in global triangle vector
		while (triPos < globalTriangles.size()) {
			Meshlet m(this, in.primitiveLimit, in.vertexLimit);
			while (triPos < globalTriangles.size() && m.canInsertTriangle(globalTriangles[triPos])) {
				m.insertTriangle(globalTriangles[triPos]);
				triPos++;
			}
			out.meshlets.push_back(m);
		}
	} else {
		uint32_t triPos = 0; // current position in global triangle vector
		while (triPos < globalTriangles.size()) {
			Meshlet m(this, in.primitiveLimit, in.vertexLimit);
			for (int i = 0; i < numTrianglesPerMeshlet; i++) {
				if (triPos >= globalTriangles.size()) continue;
				m.insertTriangle(globalTriangles[triPos]);
				triPos++;
			}
			out.meshlets.push_back(m);
		}
	}
	verifyMeshletAdjacency(true);
}

void MeshletsForMesh::applyMeshletAlgorithmGreedy(MeshletIn2& in, MeshletOut2& out, bool squeeze)
{
	Log("Meshlet algorithm greedy started for " << in.vertices.size() << " vertices and " << in.indices.size() << " indices" << std::endl);
	std::queue<GlobalMeshletVertex*> queue;
	std::unordered_map<uint32_t, unsigned char> used;
	Meshlet m(this, in.primitiveLimit, in.vertexLimit);
	for (auto& vertex : this->globalVertices) {
		if (vertex.usedInMeshlet) continue;
		queue.push(&vertex);
		while (!queue.empty()) {
			auto* curVertex = queue.front();
            queue.pop();
			for (auto triangleIndex : curVertex->neighbourTriangles) {
				//Log("Processing triangle " << triangleIndex << " for vertex " << curVertex->globalIndex << std::endl);
				auto& triangle = this->globalTriangles[triangleIndex];
				if (triangle.usedInMeshlet) continue;
				for (auto idx : triangle.indices) {
                    //Log("  process vertex " << idx << " of triangle index " << triangleIndex << std::endl);
                    auto& vert = this->globalVertices[idx];
					if (!vert.usedInMeshlet) {
                        queue.push(&vert);
					}
				}
				if (!m.canInsertTriangle(triangle)) {
					if (squeeze && m.triangles.size() < in.primitiveLimit) {
                        // try to find triangles that only use already included vertices
                        vector<uint32_t> borderTriangleIndices; // indices into this->globalTriangles
						calcMeshletBorder(borderTriangleIndices, m);
						for (auto borderTriangleIndex : borderTriangleIndices) {
							auto& borderTriangle = this->globalTriangles[borderTriangleIndex];
							if (m.canInsertTriangle(borderTriangle)) {
								//Log("Squeeze triangle " << borderTriangleIndex << " into meshlet." << endl);
								borderTriangle.usedInMeshlet = true;
								m.insertTriangle(borderTriangle);
								// mark triangle as used in meshlet
							}
                        }
					}
					// meshlet full
                    queue.push(curVertex); // reinsert current vertex to continue with it in the next meshlet
					out.meshlets.push_back(m);
					m.reset();
				}
                // before inserting, mark the triangles and its vertices as being used in a meshlet
                triangle.usedInMeshlet = true;
                m.insertTriangle(triangle);
				//Log("Insert triangle " << triangleIndex << endl);
			}
            // after processing all triangles for this vertex, mark it as used in a meshlet
            curVertex->usedInMeshlet = true;
		}
	}
    // we have to push the (non full) last meshlet if it has triangles in it
	if (m.triangles.size() > 0) {
		out.meshlets.push_back(m);
	}
}

void MeshletsForMesh::calcMeshletBorder(vector<uint32_t>& borderTriangleIndices, Meshlet& m)
{
    borderTriangleIndices.clear();
	// iterate through all vertices inside the meshlet and add neighbour triangles not already in
	for (auto& v : m.vertices) {
		//Log("Vertex " << v->globalIndex << " has " << v->neighbourTriangles.size() << " neighbour triangles." << endl);
		for (auto triangleIndex : v->neighbourTriangles) {
			auto& triangle = globalTriangles[triangleIndex];
			if (triangle.usedInMeshlet) continue; // already in meshlet
			// check if triangle is already in borderTriangleIndices
			if (std::find(borderTriangleIndices.begin(), borderTriangleIndices.end(), triangleIndex) == borderTriangleIndices.end()) {
				borderTriangleIndices.push_back(triangleIndex);
			}
		}
    }
}

void MeshletsForMesh::fillMeshletOutputBuffers(MeshletIn2& in, MeshletOut2& out)
{
	// first, we count how many indices we need for the meshlets:
	uint32_t totalIndices = 0;
	for (auto& m : out.meshlets) {
		assert(m.vertices.size() <= in.vertexLimit); // we limit the number of vertices per meshlet to 256
		//Log("MeshletOld " << " has " << m.verticesIndices.size() << " vertices and " << m.primitives.size() << " primitives." << endl);
		// we need to align global and local index buffers, so we just use the bigger size of them.
		// local index buffer size has to be multiplied by 3 to make room for 3 indices per triangle
		uint32_t additionalIndices = m.vertices.size() < m.triangles.size() ? m.triangles.size() : m.vertices.size();
		totalIndices += additionalIndices;
	}
	//Log("Total indices needed for meshlets: " << totalIndices << endl);
	// create meshlet descriptor buffer and global index buffer and local index buffer

	// DEBUG remove all but first element of mesh->meshlets
	// Remove all but the n-th element of mesh->meshlets
	//int n = 500;
	//if (mesh->meshlets.size() > 1 && n < mesh->meshlets.size()) {
	//	auto keep = mesh->meshlets[n];
	//	mesh->meshlets.clear();
	//	mesh->meshlets.push_back(keep);
	//}
	out.outMeshletDesc.resize(out.meshlets.size());
	out.outGlobalIndexBuffer.resize(totalIndices);
	out.outLocalIndexPrimitivesBuffer.resize(totalIndices * 3);
	uint32_t indexBufferOffset = 0;
	for (size_t i = 0; i < out.meshlets.size(); ++i) {
		auto& m = out.meshlets[i];
		uint8_t vp = 0; // default rendering mode
		if (m.debugColors) {
			vp = 0x01; // use debug colors
		}
		PBRShader::PackedMeshletDesc packed = PBRShader::PackedMeshletDesc::pack(0x123456789ABC, m.vertices.size(), m.triangles.size(), vp, indexBufferOffset, 0xABCDEF);
		out.outMeshletDesc[i] = packed;
		// fill global index buffers:
		for (size_t j = 0; j < m.vertices.size(); ++j) {
			assert(j < 256);
			out.outGlobalIndexBuffer[indexBufferOffset + j] = m.vertices[j]->globalIndex;
			//mesh->outLocalIndexPrimitivesBuffer[indexBufferOffset + j] = j; // local index is just the index in the meshlet
		}
		// fill local primitives index buffers:
		for (size_t j = 0; j < m.triangles.size(); ++j) {
			assert(j < 256);
			auto& triangle = m.triangles[j];
			out.outLocalIndexPrimitivesBuffer[(indexBufferOffset + j) * 3] = triangle.indices[0];
			out.outLocalIndexPrimitivesBuffer[(indexBufferOffset + j) * 3 + 1] = triangle.indices[1];
			out.outLocalIndexPrimitivesBuffer[(indexBufferOffset + j) * 3 + 2] = triangle.indices[2];
		}
		uint32_t additionalIndices = m.vertices.size() < m.triangles.size() ? m.triangles.size() : m.vertices.size();
		indexBufferOffset += additionalIndices;
	}
}

void MeshStore::generateTrianglesAndNeighbours(MeshletIn& in, MeshletIntermediate& temp)
{
	// Generate mesh structure
	temp.triangles.resize(in.indices.size() / 3);
	temp.trianglesVector.resize(temp.triangles.size());
	int unique = 0;
	int reused = 0;
	for (uint32_t i = 0; i < in.indices.size() / 3; i++) {
		MeshletOld::MeshletTriangle* t = &temp.trianglesVector[i];
		t->id = i;
		temp.triangles[i] = t;
		for (uint32_t j = 0; j < 3; j++) {
			auto lookup = temp.indexVertexMap.find(in.indices[i * 3 + j]);
			if (lookup != temp.indexVertexMap.end()) {
				// vertex already exists, reuse it
				lookup->second.neighbours.push_back(t);
				lookup->second.degree++;
				t->vertices.push_back(&lookup->second);
				reused++;
				//std::cout << "reused vertex " << lookup->second.index << std::endl;
			}
			else {
				MeshletOld::MeshletVertInfo v;
				v.index = in.indices[i * 3 + j];
				v.degree = 1;
				v.neighbours.push_back(t);
				temp.indexVertexMap[v.index] = v;
				t->vertices.push_back(&temp.indexVertexMap[v.index]);
				unique++;
			}

		}
	}
	Log("Mesh structure initialised" << std::endl);
	Log(unique << " vertices added " << reused << " reused " << (unique + reused) << " total" << std::endl);
	verifyAdjacency(in, temp);
	// Connect vertices
	uint32_t found;
	MeshletOld::MeshletTriangle* t;
	MeshletOld::MeshletTriangle* c;
	MeshletOld::MeshletVertInfo* v;
	MeshletOld::MeshletVertInfo* p;
	for (uint32_t i = 0; i < temp.triangles.size(); ++i) { // For each triangle
		t = temp.triangles[i];
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
	verifyAdjacency(in, temp);
}

void MeshStore::sort(MeshletIn& in, MeshletIntermediate& temp)
{
	// calculate centroids and bounding boxes for triangles
	auto& vertexBuffer = in.vertices; // use the original vertex buffer for sorting
	glm::vec3 min{ FLT_MAX };
	glm::vec3 max{ FLT_MIN };
	for (MeshletOld::MeshletTriangle& tri : temp.trianglesVector) {
		min = glm::min(min, vertexBuffer[tri.vertices[0]->index].pos);
		min = glm::min(min, vertexBuffer[tri.vertices[1]->index].pos);
		min = glm::min(min, vertexBuffer[tri.vertices[2]->index].pos);
		max = glm::max(max, vertexBuffer[tri.vertices[0]->index].pos);
		max = glm::max(max, vertexBuffer[tri.vertices[1]->index].pos);
		max = glm::max(max, vertexBuffer[tri.vertices[2]->index].pos);

		glm::vec3 centroid = (vertexBuffer[tri.vertices[0]->index].pos + vertexBuffer[tri.vertices[1]->index].pos + vertexBuffer[tri.vertices[2]->index].pos) / 3.0f;
		tri.centroid[0] = centroid.x;
		tri.centroid[1] = centroid.y;
		tri.centroid[2] = centroid.z;
	}

	// use the same axis info to sort vertices
	glm::vec3 axis = glm::abs(max - min);


	temp.vertsVector.reserve(temp.indexVertexMap.size());
	for (int i = 0; i < temp.indexVertexMap.size(); ++i) {
		temp.vertsVector.push_back(&temp.indexVertexMap[i]);
	}

	if (axis.x > axis.y && axis.x > axis.z) {
		sortByPosAxis(temp.vertsVector, vertexBuffer, Axis::X);
		sortTrianglesByPosAxis(temp.triangles, vertexBuffer, Axis::X);
		//std::sort(vertsVector.begin(), vertsVector.end(), std::bind(compareVerts, std::placeholders::_1, std::placeholders::_2, vertexBuffer, 0));
		//std::sort(triangles.begin(), triangles.end(), std::bind(CompareTriangles, std::placeholders::_1, std::placeholders::_2, 0));
		std::cout << "x sorted" << std::endl;
	}
	else if (axis.y > axis.z && axis.y > axis.x) {
		sortByPosAxis(temp.vertsVector, vertexBuffer, Axis::Y);
		sortTrianglesByPosAxis(temp.triangles, vertexBuffer, Axis::Y);
		//std::sort(vertsVector.begin(), vertsVector.end(), std::bind(compareVerts, std::placeholders::_1, std::placeholders::_2, vertexBuffer, 1));
		//std::sort(triangles.begin(), triangles.end(), std::bind(CompareTriangles, std::placeholders::_1, std::placeholders::_2, 1));
		std::cout << "y sorted" << std::endl;
	}
	else {
		sortByPosAxis(temp.vertsVector, vertexBuffer, Axis::Z);
		sortTrianglesByPosAxis(temp.triangles, vertexBuffer, Axis::Z);
		//std::sort(vertsVector.begin(), vertsVector.end(), std::bind(compareVerts, std::placeholders::_1, std::placeholders::_2, vertexBuffer, 2));
		//std::sort(triangles.begin(), triangles.end(), std::bind(CompareTriangles, std::placeholders::_1, std::placeholders::_2, 2));
		std::cout << "z sorted" << std::endl;
	}
}

void MeshStore::calculateMeshlets(std::string id, uint32_t meshlet_flags, uint32_t vertexLimit, uint32_t primitiveLimit)
{
#   if defined(DEBUG)
    checkVertexDuplication(id);
#   endif
	assert(primitiveLimit < GLEXT_MESHLET_PRIMITIVE_COUNT); // we need one more primitive for adding the 'rest'
	assert(vertexLimit <= GLEXT_MESHLET_VERTEX_COUNT);

	MeshInfo* mesh = getMesh(id);
	//mesh->
	// min	[-0.040992 -0.046309 -0.053326]	glm::vec<3,float,0>
	// max	[0.040992 0.067943 0.132763]	glm::vec<3,float,0>
	BoundingBox box;
	mesh->getBoundingBox(box);
	Log("bounding box min: " << box.min.x << " " << box.min.y << " " << box.min.z << endl);
	Log("bounding box max: " << box.max.x << " " << box.max.y << " " << box.max.z << endl);

	// new impl
    MeshletIn2 in2{ mesh->vertices, mesh->indices, primitiveLimit, vertexLimit };
	MeshletOut2 out2{ mesh->meshletsForMesh.meshlets, mesh->outMeshletDesc, mesh->outLocalIndexPrimitivesBuffer, mesh->outGlobalIndexBuffer };
	mesh->meshletsForMesh.calculateTrianglesAndNeighbours(in2);
	//mesh->meshletsForMesh.applyMeshletAlgorithmSimple(in2, out2);
	mesh->meshletsForMesh.applyMeshletAlgorithmGreedy(in2, out2, true);
	if (mesh->flags.hasFlag(MeshFlags::MESHLET_DEBUG_COLORS)) {
		applyDebugMeshletColorsToVertices(mesh);
		applyDebugMeshletColorsToMeshlets(mesh);
	}
	mesh->meshletsForMesh.fillMeshletOutputBuffers(in2, out2);
    logMeshletStats(mesh);
	return;

	// old impl
	MeshletIn in { mesh->vertices, mesh->indices, primitiveLimit, vertexLimit };
	MeshletOut out { mesh->meshlets , mesh->outMeshletDesc, mesh->outLocalIndexPrimitivesBuffer, mesh->outGlobalIndexBuffer};
	std::unordered_map<uint32_t, MeshletOld::MeshletVertInfo> indexVertexMap;
	std::vector<MeshletOld::MeshletTriangle*> triangles; // pointers to triangles, used in algorithms
	std::vector<MeshletOld::MeshletTriangle> trianglesVector; // base storage for triangles
	std::vector<MeshletOld::MeshletVertInfo*> vertsVector; // meshlet vertex info, used to store vertex indices and neighbours
	std::vector<uint32_t> meshletVertexIndices; // indices into vertices
	MeshletIntermediate temp{ trianglesVector, indexVertexMap, triangles, vertsVector, meshletVertexIndices};
    generateTrianglesAndNeighbours(in, temp);
    sort(in, temp);
    prepareMeshletVertexIndices(temp);

	if (meshlet_flags & static_cast<uint32_t>(MeshletFlags::MESHLET_ALG_SIMPLE)) {
        applyMeshletAlgorithmSimple(in, out);
        //applyMeshletAlgorithmSimpleOnSortedTriangles(in, temp, out);
		//MeshletOld::applyMeshletAlgorithmGreedyVerts(
		//	temp.indexVertexMap, temp.vertsVector, temp.triangles, out.meshlets, in.vertices, primitiveLimit, vertexLimit
		//);

	} else if (meshlet_flags & static_cast<uint32_t>(MeshletFlags::MESHLET_ALG_GREEDY_VERT)) {
		applyMeshletAlgorithmGreedyVerts(in, temp, out);
	}
	if (mesh->flags.hasFlag(MeshFlags::MESHLET_DEBUG_COLORS)) {
        applyDebugMeshletColorsToMeshlets(mesh);
	}
    int from = 0;
    int to = 9;
	// only keep meshlets from .. to
	if (false) {
		Log("Meshlets created with simple algorithm, keeping meshlets " << from << " to " << to << endl);
		out.meshlets.erase(out.meshlets.begin(), out.meshlets.begin() + from);
		out.meshlets.erase(out.meshlets.begin() + to, out.meshlets.end());
	}
	logMeshletStatsOld(mesh);
    fillMeshletOutputBuffers(in, out);

	int n = 14500;
	n = 185 * 4;
    //markVertexOfTriangle(n, mesh);
    logTriangleFromGlTF(n, mesh);
    logTriangleFromMeshlets(n, mesh);
    logTriangleFromMeshletBuffers(n, mesh);
}

void MeshStore::prepareMeshletVertexIndices(MeshletIntermediate& temp)
{
	temp.meshletVertexIndices.clear();
	temp.meshletVertexIndices.reserve(temp.triangles.size() * 3);
	for (auto& tri : temp.trianglesVector) {
		//Log("Triangle " << tri.id << " centroid: " << tri.centroid[0] << " " << tri.centroid[1] << " " << tri.centroid[2] << endl);
		assert(tri.vertices.size() == 3);
		auto idx = tri.vertices[0]->index;
		temp.meshletVertexIndices.push_back(idx);
		idx = tri.vertices[1]->index;
		temp.meshletVertexIndices.push_back(idx);
		idx = tri.vertices[2]->index;
		temp.meshletVertexIndices.push_back(idx);
	}
}

void MeshStore::logVertex(PBRShader::Vertex& v)
{
	Log("Vertex: pos: " << v.pos.x << " " << v.pos.y << " " << v.pos.z
		<< ", normal: " << v.normal.x << " " << v.normal.y << " " << v.normal.z
		<< ", color: " << v.color.x << " " << v.color.y << " " << v.color.z
		<< ", uv: " << v.uv0.x << " " << v.uv0.y
        << endl);
}

void MeshStore::logVertexIndex(PBRShader::Vertex& v, vector<PBRShader::Vertex>& vertices)
{
	Log("Vertex occurs at index ");
	for (size_t i = 0; i < vertices.size(); i++) {
		if (vertices[i] == v) {
			Log(" " << i);
		}
	}
	Log("\n");
}

void MeshStore::markVertexOfTriangle(int num, MeshInfo* mesh)
{
	for (int i = 0; i < 3; ++i) {
		auto& v = mesh->vertices[mesh->indices[num * 3 + i]];
        v.color.x = 0.21f * (i+1); // just mark the vertex with a color
	}
}

void MeshStore::logTriangleFromGlTF(int num, MeshInfo* mesh)
{
	Log("Triangle " << num << ":" << endl);
	for (int i = 0; i < 3; ++i) {
		auto& v = mesh->vertices[mesh->indices[num * 3 + i]];
		Log("  Vertex " << i << " "); logVertex(v);
	}
}

void MeshStore::logTriangleFromMeshletBuffers(int num, MeshInfo* mesh)
{
	// iterate through triangles to find triangle number num.
	// use only info from mesh->outMeshletDesc, mesh->outLocalIndexPrimitivesBuffer, mesh->outGlobalIndexBuffer
	//obj->mesh->outMeshletDesc, obj->mesh->outLocalIndexPrimitivesBuffer, obj->mesh->outGlobalIndexBuffer, obj->mesh->vertices
    auto& meshletDesc = mesh->outMeshletDesc;
    auto& localIndexPrimitivesBuffer = mesh->outLocalIndexPrimitivesBuffer;
    auto& globalIndexBuffer = mesh->outGlobalIndexBuffer;
    auto& vertices = mesh->vertices;
	int count = 0;
	for (int meshletIndex = 0; meshletIndex < meshletDesc.size(); meshletIndex++) {
		PBRShader::PackedMeshletDesc& packed = meshletDesc[meshletIndex];
		auto meshletOffset = packed.getIndexBufferOffset();
		for (int i = 0; i < packed.getNumPrimitives(); ++i) {
			if (count++ != num) {
                continue; // not the triangle we are looking for
            }
			// index into local index buffer:
			auto localPrimBufferIndex = meshletOffset * 3 + i * 3;
			auto localPrimIndex0 = localIndexPrimitivesBuffer[localPrimBufferIndex];
			auto localPrimIndex1 = localIndexPrimitivesBuffer[localPrimBufferIndex + 1];
			auto localPrimIndex2 = localIndexPrimitivesBuffer[localPrimBufferIndex + 2];

			auto v0 = globalIndexBuffer[meshletOffset + localPrimIndex0];
			auto v1 = globalIndexBuffer[meshletOffset + localPrimIndex1];
			auto v2 = globalIndexBuffer[meshletOffset + localPrimIndex2];
			// v0, v1, v2 are now absolute indices in the vertex buffer
            Log("Triangle " << num << " in meshlet " << meshletIndex << ":" << endl);
            Log("  Vertex 0 "); logVertex(vertices[v0]);
            Log("  Vertex 1 "); logVertex(vertices[v1]);
            Log("  Vertex 2 "); logVertex(vertices[v2]);
		}
	}


}

void MeshStore::logTriangleFromMeshlets(int num, MeshInfo* mesh)
{
    // iterate through meshlets and find the triangle with the given number
	int i = 0;
	for (auto& m : mesh->meshlets) {
		if (num < i + m.primitives.size()) {
			// found the meshlet containing the triangle
			int localIndex = num - i;
			auto& triangle = m.primitives[localIndex];
			Log("Triangle " << num << " in meshlet " << &m << ":" << endl);
			for (int j = 0; j < 3; ++j) {
				auto& v = mesh->vertices[m.verticesIndices[triangle[j]]];
				Log("  Vertex " << j << " "); logVertex(v);
			}
			return;
		}
		i += m.primitives.size();
    }
}

void MeshStore::fillMeshletOutputBuffers(MeshletIn& in, MeshletOut& out)
{
	// first, we count how many indices we need for the meshlets:
	uint32_t totalIndices = 0;
	for (auto& m : out.meshlets) {
		assert(m.verticesIndices.size() <= in.vertexLimit); // we limit the number of vertices per meshlet to 256
		//Log("MeshletOld " << " has " << m.verticesIndices.size() << " vertices and " << m.primitives.size() << " primitives." << endl);
		// we need to align global and local index buffers, so we just use the bigger size of them.
		// local index buffer size has to be multiplied by 3 to make room for 3 indices per triangle
		uint32_t additionalIndices = m.verticesIndices.size() < m.primitives.size() ? m.primitives.size() : m.verticesIndices.size();
		totalIndices += additionalIndices;
	}
	//Log("Total indices needed for meshlets: " << totalIndices << endl);
	// create meshlet descriptor buffer and global index buffer and local index buffer

	// DEBUG remove all but first element of mesh->meshlets
	// Remove all but the n-th element of mesh->meshlets
	//int n = 500;
	//if (mesh->meshlets.size() > 1 && n < mesh->meshlets.size()) {
	//	auto keep = mesh->meshlets[n];
	//	mesh->meshlets.clear();
	//	mesh->meshlets.push_back(keep);
	//}
	out.outMeshletDesc.resize(out.meshlets.size());
	out.outGlobalIndexBuffer.resize(totalIndices);
	out.outLocalIndexPrimitivesBuffer.resize(totalIndices * 3);
	uint32_t indexBufferOffset = 0;
	for (size_t i = 0; i < out.meshlets.size(); ++i) {
		auto& m = out.meshlets[i];
		//PBRShader::PackedMeshletDesc packed = PBRShader::PackedMeshletDesc::pack(0x123456789ABC, 12, 34, 56, 101, 0xABCDEF);
		//PBRShader::PackedMeshletDesc packed = PBRShader::PackedMeshletDesc::pack(0x00, 0, 0, 0, 0x00, 0x00);
		//PBRShader::PackedMeshletDesc packed = PBRShader::PackedMeshletDesc::pack(-1, -1, -1, -1, -1, -1);
        uint8_t vp = 0; // default rendering mode
        if (m.debugColors) {
			vp = 0x01; // use debug colors
        }
		PBRShader::PackedMeshletDesc packed = PBRShader::PackedMeshletDesc::pack(0x123456789ABC, m.verticesIndices.size(), m.primitives.size(), vp, indexBufferOffset, 0xABCDEF);
		out.outMeshletDesc[i] = packed;
		// fill global index buffers:
		for (size_t j = 0; j < m.verticesIndices.size(); ++j) {
			assert(j < 256);
			out.outGlobalIndexBuffer[indexBufferOffset + j] = m.verticesIndices[j];
			//mesh->outLocalIndexPrimitivesBuffer[indexBufferOffset + j] = j; // local index is just the index in the meshlet
		}
		// fill local primitives index buffers:
		for (size_t j = 0; j < m.primitives.size(); ++j) {
			assert(j < 256);
			auto& triangle = m.primitives[j];
			out.outLocalIndexPrimitivesBuffer[(indexBufferOffset + j) * 3] = triangle[0];
			out.outLocalIndexPrimitivesBuffer[(indexBufferOffset + j) * 3 + 1] = triangle[1];
			out.outLocalIndexPrimitivesBuffer[(indexBufferOffset + j) * 3 + 2] = triangle[2];
		}
		uint32_t additionalIndices = m.verticesIndices.size() < m.primitives.size() ? m.primitives.size() : m.verticesIndices.size();
		indexBufferOffset += additionalIndices;
	}
}

void MeshStore::applyMeshletAlgorithmSimple(MeshletIn& in, MeshletOut& out)
{
    Log("MeshletOld algorithm simple started for " << in.vertices.size() << " vertices and " << in.indices.size() << " indices" << std::endl);
	// assert that we have an indices consistent with triangle layout (must be multiple of 3)
	assert(in.indices.size() % 3 == 0);
	const int NUM_TRIANGLES_IN_MESHLET = 4;
    uint32_t indexBufferPos = 0; // current position in index buffer
	while (indexBufferPos < in.indices.size()) {
		MeshletOld m;
		for (int i = 0; i < NUM_TRIANGLES_IN_MESHLET; i++) {
			if (indexBufferPos >= in.indices.size()) continue;
			m.insert(&in.indices[indexBufferPos]);
			indexBufferPos += 3;
		}
		out.meshlets.push_back(m);
	}
}

void MeshStore::applyMeshletAlgorithmSimpleOnSortedTriangles(MeshletIn& in, MeshletIntermediate& temp, MeshletOut& out)
{
	Log("MeshletOld algorithm simple started for " << temp.triangles.size() << " triangles" << std::endl);
	// assert that we have an indices consistent with triangle layout (must be multiple of 3)
	const int NUM_TRIANGLES_IN_MESHLET = 20;
	uint32_t trianglePos = 0; // current position in triangle list
	while (trianglePos < temp.triangles.size()) {
		MeshletOld m;
		for (int i = 0; i < NUM_TRIANGLES_IN_MESHLET; i++) {
			if (trianglePos >= temp.triangles.size()) continue;
			auto& verts = temp.triangles[trianglePos]->vertices;
			uint32_t idx[3];
			idx[0] = verts[0]->index;
			idx[1] = verts[1]->index;
			idx[2] = verts[2]->index;
			m.insert(idx);
			trianglePos++;
		}
		out.meshlets.push_back(m);
	}
}

void MeshStore::applyMeshletAlgorithmGreedyVerts(MeshletIn& in, MeshletIntermediate& temp, MeshletOut& out)
{
	auto& indexVertexMap = temp.indexVertexMap;
	auto& vertsVector = temp.vertsVector;
	auto& triangles = temp.triangles;
	auto& meshlets = out.meshlets;
	const auto& vertexBuffer = in.vertices;
	auto primitiveLimit = in.primitiveLimit;
	auto vertexLimit = in.vertexLimit;

	std::queue<MeshletOld::MeshletVertInfo*> priorityQueue;
	std::unordered_map<uint32_t, unsigned char> used;
	MeshletOld cache;
	for (uint32_t i = 0; i < vertsVector.size(); i++) {
		MeshletOld::MeshletVertInfo* vert = vertsVector[i];
		if (used.find(vert->index) != used.end()) continue;
		// reset
		priorityQueue.push(vert);

		// add triangles to cache until it is full
		while (!priorityQueue.empty()) {
			// pop current triangle
			MeshletOld::MeshletVertInfo* vert = priorityQueue.front();

			for (MeshletOld::MeshletTriangle* tri : vert->neighbours) {
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
						for (MeshletOld::MeshletTriangle* tri : indexVertexMap[cache.verticesIndices[v]].neighbours) {
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

void MeshStore::calculateMeshletsX(std::string id, uint32_t vertexLimit, uint32_t primitiveLimit)
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

	std::unordered_map<uint32_t, MeshletOld::MeshletVertInfo> indexVertexMap;
    std::vector<MeshletOld::MeshletTriangle*> triangles; // pointers to triangles, used in algorithms
    std::vector<MeshletOld::MeshletTriangle> trianglesVector; // base storage for triangles

	// Generate mesh structure
	triangles.resize(uniqueIndices.size() / 3);
	trianglesVector.resize(triangles.size());
	int unique = 0;
	int reused = 0;
	for (uint32_t i = 0; i < uniqueIndices.size() / 3; i++) {
		MeshletOld::MeshletTriangle* t = &trianglesVector[i];
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
                MeshletOld::MeshletVertInfo v;
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
	MeshletOld::MeshletTriangle* t;
	MeshletOld::MeshletTriangle* c;
	MeshletOld::MeshletVertInfo* v;
	MeshletOld::MeshletVertInfo* p;
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

    // replace vertexBuffer with unique vertices: TODO if unique vertex count is same we could skip this, check once we have more models tested
    if (uniqueVertices.size() == mesh->vertices.size()) {
		Log("WARNING: Mesh " << id << " has same vertex count as unique vertices, replacement should be skipped." << endl);
	}
	// TODO rethink greedy algo: probably need to keep the 3 verts that form triangle together while creating 'unique' vertices :-)
	for (auto& v : mesh->vertices) {
		for (auto& uv : uniqueVertices) {
			if (v.pos == uv.pos) {
				if (v.color != uv.color) {
					Log("Mesh " << id << " has different color for the same position" << endl);
				}
				if (v.normal != uv.normal) {
					Log("Mesh " << id << " has different normal for the same position" << endl);
				}
				if (v.uv0 != uv.uv0) {
					Log("Mesh " << id << " has different uv0 for the same position" << endl);
				}
				if (v.uv1 != uv.uv1) {
					Log("Mesh " << id << " has different uv1 for the same position" << endl);
				}
			}
		}
    }
	bool use_Old_Vert = true;
	if (!use_Old_Vert) {
		mesh->vertices.clear();
		mesh->vertices.reserve(uniqueVertices.size());
		for (auto& v : uniqueVertices) {
			mesh->vertices.push_back(v);
		}
	}


    // calculate centroids and bounding boxes for triangles
    auto& vertexBuffer = mesh->vertices; // use the original vertex buffer for sorting
	glm::vec3 min{ FLT_MAX };
	glm::vec3 max{ FLT_MIN };
	for (MeshletOld::MeshletTriangle& tri : trianglesVector) {
		min = glm::min(min, vertexBuffer[tri.vertices[0]->index].pos);
		min = glm::min(min, vertexBuffer[tri.vertices[1]->index].pos);
		min = glm::min(min, vertexBuffer[tri.vertices[2]->index].pos);
		max = glm::max(max, vertexBuffer[tri.vertices[0]->index].pos);
		max = glm::max(max, vertexBuffer[tri.vertices[1]->index].pos);
		max = glm::max(max, vertexBuffer[tri.vertices[2]->index].pos);

		glm::vec3 centroid = (vertexBuffer[tri.vertices[0]->index].pos + vertexBuffer[tri.vertices[1]->index].pos + vertexBuffer[tri.vertices[2]->index].pos) / 3.0f;
		tri.centroid[0] = centroid.x;
		tri.centroid[1] = centroid.y;
		tri.centroid[2] = centroid.z;
	}

	// use the same axis info to sort vertices
	glm::vec3 axis = glm::abs(max - min);


	mesh->vertsVector.reserve(indexVertexMap.size());
	for (int i = 0; i < indexVertexMap.size(); ++i) {
		mesh->vertsVector.push_back(&indexVertexMap[i]);
	}

	if (axis.x > axis.y && axis.x > axis.z) {
        sortByPosAxis(mesh->vertsVector, vertexBuffer, Axis::X);
        sortTrianglesByPosAxis(triangles, vertexBuffer, Axis::X);
		//std::sort(vertsVector.begin(), vertsVector.end(), std::bind(compareVerts, std::placeholders::_1, std::placeholders::_2, vertexBuffer, 0));
		//std::sort(triangles.begin(), triangles.end(), std::bind(CompareTriangles, std::placeholders::_1, std::placeholders::_2, 0));
		std::cout << "x sorted" << std::endl;
	}
	else if (axis.y > axis.z && axis.y > axis.x) {
		sortByPosAxis(mesh->vertsVector, vertexBuffer, Axis::Y);
		sortTrianglesByPosAxis(triangles, vertexBuffer, Axis::Y);
		//std::sort(vertsVector.begin(), vertsVector.end(), std::bind(compareVerts, std::placeholders::_1, std::placeholders::_2, vertexBuffer, 1));
		//std::sort(triangles.begin(), triangles.end(), std::bind(CompareTriangles, std::placeholders::_1, std::placeholders::_2, 1));
		std::cout << "y sorted" << std::endl;
	}
	else {
		sortByPosAxis(mesh->vertsVector, vertexBuffer, Axis::Z);
		sortTrianglesByPosAxis(triangles, vertexBuffer, Axis::Z);
		//std::sort(vertsVector.begin(), vertsVector.end(), std::bind(compareVerts, std::placeholders::_1, std::placeholders::_2, vertexBuffer, 2));
		//std::sort(triangles.begin(), triangles.end(), std::bind(CompareTriangles, std::placeholders::_1, std::placeholders::_2, 2));
		std::cout << "z sorted" << std::endl;
	}
	mesh->meshletVertexIndices.reserve(mesh->vertsVector.size());
	//int vertCount = 0, colIndex = 0;
	uint32_t vIndexMax = 0;
    uint32_t vIndexMin = UINT32_MAX;
	for (auto& v : mesh->vertsVector) {
		//vertCount++;
		//std::cout << "vertex " << v->index << " pos: " << vertexBuffer[v->index].pos.x << " " << vertexBuffer[v->index].pos.y << " " << vertexBuffer[v->index].pos.z << std::endl;
        mesh->meshletVertexIndices.push_back(v->index);
		if (v->index > vIndexMax) {
			vIndexMax = v->index;
        }
		if (v->index < vIndexMin) {
			vIndexMin = v->index;
		}
	}
    Log("MeshletOld vertex indices min: " << vIndexMin << " max: " << vIndexMax << " out of total vertices from file: " << vertexBuffer.size() << endl);
	//return;
	mesh->meshletVertexIndices.clear();
	mesh->meshletVertexIndices.reserve(triangles.size()*3);
	for (auto& tri : trianglesVector) {
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
    }
	MeshletOld::applyMeshletAlgorithmGreedyVerts(
		indexVertexMap, mesh->vertsVector, triangles, mesh->meshlets, vertexBuffer, primitiveLimit, vertexLimit
    );
    //applyDebugMeshletColorsToVertices(mesh);
	logMeshletStatsOld(mesh);
    // create meshlet descriptors and buffers:
    // first, we count how many indices we need for the meshlets:
    uint32_t totalIndices = 0;
	for (auto& m : mesh->meshlets) {
        assert(m.verticesIndices.size() <= vertexLimit); // we limit the number of vertices per meshlet to 256
        //Log("MeshletOld " << " has " << m.verticesIndices.size() << " vertices and " << m.primitives.size() << " primitives." << endl);
		// we need to align global and local indeex buffers, so we just use the bigger size of them.
        // local index buffer size has to be multiplied by 3 to make room for 3 indices per triangle
        uint32_t additionalIndices = m.verticesIndices.size() < m.primitives.size() ? m.primitives.size() : m.verticesIndices.size();
        totalIndices += additionalIndices;
	}
    //Log("Total indices needed for meshlets: " << totalIndices << endl);
	// create meshlet descriptor buffer and global index buffer and local index buffer

	// DEBUG remove all but first element of mesh->meshlets
	// Remove all but the n-th element of mesh->meshlets
	//int n = 500;
	//if (mesh->meshlets.size() > 1 && n < mesh->meshlets.size()) {
	//	auto keep = mesh->meshlets[n];
	//	mesh->meshlets.clear();
	//	mesh->meshlets.push_back(keep);
	//}
	mesh->outMeshletDesc.resize(mesh->meshlets.size());
	mesh->outGlobalIndexBuffer.resize(totalIndices);
    mesh->outLocalIndexPrimitivesBuffer.resize(totalIndices * 3);
	uint32_t indexBufferOffset = 0;
	for (size_t i = 0; i < mesh->meshlets.size(); ++i) {
		auto& m = mesh->meshlets[i];
		//PBRShader::PackedMeshletDesc packed = PBRShader::PackedMeshletDesc::pack(0x123456789ABC, 12, 34, 56, 101, 0xABCDEF);
		//PBRShader::PackedMeshletDesc packed = PBRShader::PackedMeshletDesc::pack(0x00, 0, 0, 0, 0x00, 0x00);
		//PBRShader::PackedMeshletDesc packed = PBRShader::PackedMeshletDesc::pack(-1, -1, -1, -1, -1, -1);
		PBRShader::PackedMeshletDesc packed = PBRShader::PackedMeshletDesc::pack(0x123456789ABC, m.verticesIndices.size(), m.primitives.size(), 56, indexBufferOffset, 0xABCDEF);
		mesh->outMeshletDesc[i] = packed;
		// fill global index buffers:
		for (size_t j = 0; j < m.verticesIndices.size(); ++j) {
			assert(j < 256);
			mesh->outGlobalIndexBuffer[indexBufferOffset + j] = m.verticesIndices[j];
			//mesh->outLocalIndexPrimitivesBuffer[indexBufferOffset + j] = j; // local index is just the index in the meshlet
		}
		// fill local primitives index buffers:
		for (size_t j = 0; j < m.primitives.size(); ++j) {
			assert(j < 256);
            auto& triangle = m.primitives[j];
			mesh->outLocalIndexPrimitivesBuffer[(indexBufferOffset + j) * 3] = triangle[0];
			mesh->outLocalIndexPrimitivesBuffer[(indexBufferOffset + j) * 3 + 1] = triangle[1];
			mesh->outLocalIndexPrimitivesBuffer[(indexBufferOffset + j) * 3 + 2] = triangle[2];
		}
		uint32_t additionalIndices = m.verticesIndices.size() < m.primitives.size() ? m.primitives.size() : m.verticesIndices.size();
		indexBufferOffset += additionalIndices;
	}
}

void MeshStore::logMeshletStatsOld(MeshInfo* mesh)
{
	Log("MeshletOld stats for mesh " << mesh->id << endl);
	Log("  Meshlets: " << mesh->meshlets.size() << endl);
	//Log("  MeshletOld descriptors: " << mesh->outMeshletDesc.size() << endl);
	//Log("  MeshletOld triangles: " << mesh->meshlets.size() * 12 << endl); // each meshlet has 12 triangles
	int localIndexCount = 0; // count indices used for all meshlets
	int avgVertsPerMeshlet = 0;
	int avgPrimsPerMeshlet = 0;
	static auto col = engine->util.generateColorPalette256();

	for (auto& m : mesh->meshlets) {
		localIndexCount += m.verticesIndices.size();
		avgPrimsPerMeshlet += m.primitives.size();
		avgVertsPerMeshlet += m.verticesIndices.size();
		assert(m.verticesIndices.size() <= 256); // we limit the number of vertices per meshlet to 256
		//for (auto& v : m.primitives) {
		//	// v is a triangle
		//	assert(v[0] < m.verticesIndices.size() && v[1] < m.verticesIndices.size() && v[2] < m.verticesIndices.size());
  //          localIndexCount += m.verticesIndices.size();
		//	auto& v0 = m.verticesIndices[v[0]];
		//	auto& v1 = m.verticesIndices[v[1]];
		//	auto& v2 = m.verticesIndices[v[2]];
		//	assert(v0 < mesh->vertices.size() && v1 < mesh->vertices.size() && v2 < mesh->vertices.size());
		//	auto& v0pos = mesh->vertices[v0].pos;
		//	auto& v1pos = mesh->vertices[v1].pos;
		//	auto& v2pos = mesh->vertices[v2].pos;
		//	//vec3 v0posWorld = vec3(modelToWorld * vec4(v0pos, 1.0f));
		//	//vec3 v1posWorld = vec3(modelToWorld * vec4(v1pos, 1.0f));
		//	//vec3 v2posWorld = vec3(modelToWorld * vec4(v2pos, 1.0f));
		//}
	}
	avgPrimsPerMeshlet /= mesh->meshlets.size();
	avgVertsPerMeshlet /= mesh->meshlets.size();
	Log("  Average MeshletOld verts / triangles: " << avgVertsPerMeshlet << " / " << avgPrimsPerMeshlet << endl);
	Log("  local Vertex indices (b4 greedy alg): " << mesh->meshletVertexIndices.size() << endl);
	Log("  local Vertex indices needed         : " << localIndexCount << endl);
	Log("  vertices: " << mesh->vertices.size() << endl);
}

void MeshStore::logMeshletStats(MeshInfo* mesh)
{
    assert(mesh->meshletsForMesh.meshlets.size() > 0);
	Log("MeshletOld stats for mesh " << mesh->id << endl);
	Log("  Meshlets: " << mesh->meshletsForMesh.meshlets.size() << endl);
	//Log("  MeshletOld descriptors: " << mesh->outMeshletDesc.size() << endl);
	//Log("  MeshletOld triangles: " << mesh->meshlets.size() * 12 << endl); // each meshlet has 12 triangles
	int localIndexCount = 0; // count indices used for all meshlets
	int avgVertsPerMeshlet = 0;
	int avgPrimsPerMeshlet = 0;
	static auto col = engine->util.generateColorPalette256();

	for (auto& m : mesh->meshletsForMesh.meshlets) {
		localIndexCount += m.verticesIndices.size();
		avgPrimsPerMeshlet += m.triangles.size();
		avgVertsPerMeshlet += m.vertices.size();
		assert(m.vertices.size() <= 256); // we limit the number of vertices per meshlet to 256
		//for (auto& v : m.primitives) {
		//	// v is a triangle
		//	assert(v[0] < m.verticesIndices.size() && v[1] < m.verticesIndices.size() && v[2] < m.verticesIndices.size());
  //          localIndexCount += m.verticesIndices.size();
		//	auto& v0 = m.verticesIndices[v[0]];
		//	auto& v1 = m.verticesIndices[v[1]];
		//	auto& v2 = m.verticesIndices[v[2]];
		//	assert(v0 < mesh->vertices.size() && v1 < mesh->vertices.size() && v2 < mesh->vertices.size());
		//	auto& v0pos = mesh->vertices[v0].pos;
		//	auto& v1pos = mesh->vertices[v1].pos;
		//	auto& v2pos = mesh->vertices[v2].pos;
		//	//vec3 v0posWorld = vec3(modelToWorld * vec4(v0pos, 1.0f));
		//	//vec3 v1posWorld = vec3(modelToWorld * vec4(v1pos, 1.0f));
		//	//vec3 v2posWorld = vec3(modelToWorld * vec4(v2pos, 1.0f));
		//}
	}
	avgPrimsPerMeshlet /= mesh->meshletsForMesh.meshlets.size();
	avgVertsPerMeshlet /= mesh->meshletsForMesh.meshlets.size();
	Log("  Average Meshlet verts / triangles: " << avgVertsPerMeshlet << " / " << avgPrimsPerMeshlet << endl);
	Log("  local Vertex indices (b4 greedy alg): " << mesh->meshletsForMesh.indexVertexMap.size() << endl);
	Log("  local Vertex indices needed         : " << localIndexCount << endl);
	Log("  vertices: " << mesh->meshletsForMesh.globalVertices.size() << endl);
}

void MeshletOld::applyMeshletAlgorithmGreedyVerts(
	std::unordered_map<uint32_t, MeshletOld::MeshletVertInfo>& indexVertexMap, // 117008
	std::vector<MeshletOld::MeshletVertInfo*>& vertsVector, // 117008
	std::vector<MeshletOld::MeshletTriangle*>& triangles, // 231256
	std::vector<MeshletOld>& meshlets, // 0
	const std::vector<PBRShader::Vertex>& vertexBuffer, // 117008 vertices
	uint32_t primitiveLimit, uint32_t vertexLimit // 125, 64
)
{
    std::queue<MeshletOld::MeshletVertInfo*> priorityQueue;
	std::unordered_map<uint32_t, unsigned char> used;
    MeshletOld cache;
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
		Log("MeshStore: Mesh " << id << " has a good ratio of total vertices to separate vertices: " << ratio << endl);
    }
}

void MeshStore::checkVertexDuplication(std::string id)
{
	MeshInfo* mesh = getMesh(id);

	std::unordered_set<PBRShader::Vertex> uniqueVertices;
	for (auto& v : mesh->vertices) {
		if (!uniqueVertices.insert(v).second) {
			// duplicate found, log v:
			//Log("duplicate vertex: "); logVertex(v);
			//logVertexIndex(v, mesh->vertices);
		}
	}
    size_t numDuplicates = mesh->vertices.size() - uniqueVertices.size();
	if (numDuplicates > 0) {
        float percentage = (float)numDuplicates / (float)mesh->vertices.size() * 100.0f;
		Log("WARNING: Mesh " << id << " has duplicated vertices: " << numDuplicates << " (" << std::round(percentage) << "%) - you should consider cleaning up the mesh before usage." << endl);
	}
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
	//lineShader.prepareAddLines(fr);
	lineShader.addOneTime(addLines, fr);
}

void MeshStore::debugRenderMeshlet(WorldObject* obj, FrameResources& fr, glm::mat4 modelToWorld, glm::vec4 color)
{
	// get access to line shader
	auto& lineShader = engine->shaders.lineShader;
	if (!lineShader.enabled) return;
	if (!obj->enableDebugGraphics) return;

	vector<LineDef> addLines;
	
	obj->drawBoundingBox(addLines, modelToWorld, color);

	int meshletCount = 0;
	static auto col = engine->util.generateColorPalette256();

	for (auto& m : obj->mesh->meshletsForMesh.meshlets) {
        uint32_t minIndex = UINT32_MAX;
        uint32_t maxIndex = 0;
		auto color = col[meshletCount % 256]; // assign color from palette
		meshletCount++;
		for (auto& v : m.triangles) {
            // vert indices are local (range 0..256)
			assert(v.indices[0] < m.vertices.size());
			assert(v.indices[1] < m.vertices.size());
			assert(v.indices[2] < m.vertices.size());
			auto& v0 = m.vertices[v.indices[0]]->globalIndex;
			auto& v1 = m.vertices[v.indices[1]]->globalIndex;
			auto& v2 = m.vertices[v.indices[2]]->globalIndex;
			if (v0 < minIndex) minIndex = v0;
            if (v0 > maxIndex) maxIndex = v0;
            if (v1 < minIndex) minIndex = v1;
            if (v1 > maxIndex) maxIndex = v1;
			if (v2 < minIndex) minIndex = v2;
            if (v2 > maxIndex) maxIndex = v2;
			assert(v0 < obj->mesh->vertices.size() && v1 < obj->mesh->vertices.size() && v2 < obj->mesh->vertices.size());
			auto& v0pos = obj->mesh->vertices[v0].pos;
			auto& v1pos = obj->mesh->vertices[v1].pos;
			auto& v2pos = obj->mesh->vertices[v2].pos;
			vec3 v0posWorld = vec3(modelToWorld * vec4(v0pos, 1.0f));
			vec3 v1posWorld = vec3(modelToWorld * vec4(v1pos, 1.0f));
			vec3 v2posWorld = vec3(modelToWorld * vec4(v2pos, 1.0f));
			LineDef l;
			l.color = color;
			l.start = v0posWorld;
			l.end = v1posWorld;
			addLines.push_back(l);
			l.start = v1posWorld;
			l.end = v2posWorld;
			addLines.push_back(l);
			l.start = v2posWorld;
			l.end = v0posWorld;
			addLines.push_back(l);
		}
		if (maxIndex - minIndex +1 > 256) {
			//Log("WARNING: MeshletOld " << meshletCount << " local index spans more than 256 continuous indices, this is not allowed! Min index: " << minIndex << " Max index: " << maxIndex << endl);
		}
	}

	//lineShader.prepareAddLines(fr);
	lineShader.addOneTime(addLines, fr);
}

void MeshStore::debugRenderMeshletFromBuffers(WorldObject* obj, FrameResources& fr, glm::mat4 modelToWorld, int singleMeshletNum)
{
	if (!obj->enableDebugGraphics) return;

    debugRenderMeshletFromBuffers(fr, modelToWorld, obj->mesh->outMeshletDesc, obj->mesh->outLocalIndexPrimitivesBuffer, obj->mesh->outGlobalIndexBuffer, obj->mesh->vertices, singleMeshletNum);
	return;
}

void MeshStore::debugRenderMeshletFromBuffers(
	FrameResources& fr, glm::mat4 modelToWorld,
	std::vector<PBRShader::PackedMeshletDesc>& meshletDesc,
	std::vector<uint8_t>& localIndexPrimitivesBuffer,
	std::vector<uint32_t>& globalIndexBuffer,
	std::vector<PBRShader::Vertex>& vertices, int singleMeshletNum
) {
	// get access to line shader
	auto& lineShader = engine->shaders.lineShader;
	if (!lineShader.enabled) return;

	vector<LineDef> addLines;

	int meshletCount = 0;
	static auto col = engine->util.generateColorPalette256();

	for (int meshletIndex = 0; meshletIndex < meshletDesc.size(); meshletIndex++) {
		if (singleMeshletNum >= 0 && singleMeshletNum != meshletIndex) {
			continue; // skip all but the single meshlet
        }
		PBRShader::PackedMeshletDesc& packed = meshletDesc[meshletIndex];
		auto meshletOffset = packed.getIndexBufferOffset();
		auto color = col[meshletCount++ % 256]; // assign color from palette
		color = vec4(0.0f, 0.0f, 0.0f, 1.0f); // use black color for meshlets
		for (int i = 0; i < packed.getNumPrimitives(); ++i) {

			// index into local index buffer:
			auto localPrimBufferIndex = meshletOffset * 3 + i * 3;
			auto localPrimIndex0 = localIndexPrimitivesBuffer[localPrimBufferIndex];
			auto localPrimIndex1 = localIndexPrimitivesBuffer[localPrimBufferIndex + 1];
			auto localPrimIndex2 = localIndexPrimitivesBuffer[localPrimBufferIndex + 2];

			auto v0 = globalIndexBuffer[meshletOffset + localPrimIndex0];
			auto v1 = globalIndexBuffer[meshletOffset + localPrimIndex1];
			auto v2 = globalIndexBuffer[meshletOffset + localPrimIndex2];
			// v0, v1, v2 are now absolute indices in the vertex buffer
			auto& v0pos = vertices[v0].pos;
			auto& v1pos = vertices[v1].pos;
			auto& v2pos = vertices[v2].pos;
			vec3 v0posWorld = vec3(modelToWorld * vec4(v0pos, 1.0f));
			vec3 v1posWorld = vec3(modelToWorld * vec4(v1pos, 1.0f));
			vec3 v2posWorld = vec3(modelToWorld * vec4(v2pos, 1.0f));
			LineDef l;
			l.color = color;
			l.start = v0posWorld;
			l.end = v1posWorld;
			addLines.push_back(l);
			l.start = v1posWorld;
			l.end = v2posWorld;
			addLines.push_back(l);
			l.start = v2posWorld;
			l.end = v0posWorld;
			addLines.push_back(l);
		}
	}

	//lineShader.prepareAddLines(fr);
	lineShader.addOneTime(addLines, fr);
}

bool Meshlet::canInsertTriangle(const GlobalMeshletTriangle& triangle) const {

	// check if any of the incoming three vertices are already in meshlet:
	uint32_t found = 0;
	for (auto vert : vertices) {
		for (auto vert_index_incoming : triangle.indices) {
			if (vert->globalIndex == vert_index_incoming) {
				found++;
			}
		}
	}
	bool ret = (vertices.size() + 3 - found) <= maxVertexSize && (triangles.size() + 1) <= maxPrimitiveSize;
	if (ret) {
		assert(vertices.size() <= maxVertexSize);
		assert(triangles.size() <= maxPrimitiveSize);
	}
	return ret;
}

// insert triangle into meshlet, fitting checks must be done before calling this
void Meshlet::insertTriangle(GlobalMeshletTriangle& triangle) {
	//check degenerate
	if (triangle.indices[0] == triangle.indices[1] || triangle.indices[1] == triangle.indices[2] || triangle.indices[0] == triangle.indices[2]) {
		assert(false && "Degenerate triangle found, skipping");
		//return; // degenerate triangle, skip
	}

    LocalMeshletTriangle localTriangle; // holds indices < 256 into local vertex list
	uint32_t vertOfTriangle = 0;
	for (auto vert_index_incoming : triangle.indices) {
		// check if vertex is already in meshlet:
		bool found = false;
		uint32_t useIndex = -1;
		for (uint32_t vertIdx = 0; vertIdx < vertices.size(); vertIdx++) {
            GlobalMeshletVertex* vert = vertices[vertIdx];
			if (vert->globalIndex == vert_index_incoming) {
				found = true;
                useIndex = vertIdx;
				break;
			}
		}
		// if vertex is not in cache add it
		if (!found) {
			// add vertex to meshlet
			GlobalMeshletVertex* vertex = &parent->globalVertices[vert_index_incoming];
			vertices.push_back(vertex);
            useIndex = vertices.size() - 1; // use index of the newly added vertex
		}
        assert(useIndex >= 0);
        localTriangle.indices[vertOfTriangle++] = useIndex; // store local index of the vertex in triangle
	}
	// now all vertices are added to meshlet, add triangle
	triangles.push_back(localTriangle); // store pointer to triangle meshlet triangle list
}

void MeshStore::loadMeshGrid(std::string id, MeshFlagsCollection flags, std::string baseColorTextureId, int gridSize, float scale)
{
	// Create mesh info and fill with generated data
	MeshInfo meshInfo;
	meshInfo.id = id;
	meshInfo.flags = flags;

	Util::GenerateGridMesh(gridSize, meshInfo.vertices, meshInfo.indices, scale);

	if (flags.hasFlag(MeshFlags::MESH_TYPE_FLIP_WINDING_ORDER)) {
		// Flip winding order
		for (size_t i = 0; i < meshInfo.indices.size(); i += 3) {
			std::swap(meshInfo.indices[i], meshInfo.indices[i + 2]);
		}
	}

	// Set all default material values
	setDefaultMaterial(meshInfo.material);

	// Assign base color texture if provided
	if (!baseColorTextureId.empty()) {
		meshInfo.baseColorTexture = engine->textureStore.getTexture(baseColorTextureId);
		if (!meshInfo.baseColorTexture) {
			Log("WARNING: Could not find base color texture with id: " << baseColorTextureId << endl);
		}
	}

	meshInfo.available = true;
	meshes[id] = std::move(meshInfo);
}

void MeshStore::loadMeshCylinder(std::string id, MeshFlagsCollection flags, std::string baseColorTextureId, int segments, int heightDivs, float radius, float height)
{
	MeshInfo meshInfo;
	meshInfo.id = id;
	meshInfo.flags = flags;

	Util::GenerateCylinderMesh(segments, heightDivs, radius, height, meshInfo.vertices, meshInfo.indices);

	if (flags.hasFlag(MeshFlags::MESH_TYPE_FLIP_WINDING_ORDER)) {
		// Flip winding order
		for (size_t i = 0; i < meshInfo.indices.size(); i += 3) {
			std::swap(meshInfo.indices[i], meshInfo.indices[i + 2]);
		}
	}

	// Set all default material values
	setDefaultMaterial(meshInfo.material);

	// Assign base color texture if provided
	if (!baseColorTextureId.empty()) {
		meshInfo.baseColorTexture = engine->textureStore.getTexture(baseColorTextureId);
		if (!meshInfo.baseColorTexture) {
			Log("WARNING: Could not find base color texture with id: " << baseColorTextureId << endl);
		}
	}

	meshInfo.available = true;
	meshes[id] = std::move(meshInfo);
}

void MeshStore::setDefaultMaterial(PBRShader::ShaderMaterial& mat) {
	mat.baseColorFactor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	mat.metallicFactor = 0.0f;
	mat.roughnessFactor = 1.0f;
	mat.emissiveFactor = glm::vec4(0.0f);
	mat.metallicFactor = 0.0f;   // Default: non-metallic
	mat.roughnessFactor = 1.0f;   // Default: fully rough (matte)
	mat.alphaMask = 0.0f;   // Default: no alpha mask
	mat.alphaMaskCutoff = 0.5f;   // Default: standard cutoff
	mat.emissiveStrength = 1.0f;   // Default: no extra emission
}

// Add this method inside the Meshlet class
bool Meshlet::isTrianglesConnected() const {
	size_t triCount = triangles.size();
	if (triCount <= 1) return true;

	// Build adjacency list: for each triangle, store indices of adjacent triangles
	std::vector<std::vector<size_t>> adjacency(triCount);

	for (size_t i = 0; i < triCount; ++i) {
		const auto& triA = triangles[i];
		for (size_t j = i + 1; j < triCount; ++j) {
			const auto& triB = triangles[j];
			// Count shared local vertex indices
			int shared = 0;
			for (int ai = 0; ai < 3; ++ai) {
				for (int bi = 0; bi < 3; ++bi) {
					if (triA.indices[ai] == triB.indices[bi]) {
						++shared;
					}
				}
			}
			if (shared >= 2) {
				adjacency[i].push_back(j);
				adjacency[j].push_back(i);
			}
		}
	}

	// BFS to check connectivity
	std::vector<bool> visited(triCount, false);
	std::queue<size_t> q;
	q.push(0);
	visited[0] = true;
	size_t visitedCount = 1;

	while (!q.empty()) {
		size_t curr = q.front();
		q.pop();
		for (size_t neighbor : adjacency[curr]) {
			if (!visited[neighbor]) {
				visited[neighbor] = true;
				q.push(neighbor);
				++visitedCount;
			}
		}
	}

	return visitedCount == triCount;
}

bool Meshlet::isVerticesConnected() const {
	size_t vertCount = vertices.size();
	if (vertCount <= 1) return true;

	// Build adjacency list for local vertices
	std::vector<std::vector<size_t>> adjacency(vertCount);

	// For each triangle, connect its three vertices
	for (const auto& tri : triangles) {
		for (int i = 0; i < 3; ++i) {
			size_t a = tri.indices[i];
			size_t b = tri.indices[(i + 1) % 3];
			size_t c = tri.indices[(i + 2) % 3];
			// Add undirected edges
			if (a != b) adjacency[a].push_back(b);
			if (b != a) adjacency[b].push_back(a);
			if (a != c) adjacency[a].push_back(c);
			if (c != a) adjacency[c].push_back(a);
			if (b != c) adjacency[b].push_back(c);
			if (c != b) adjacency[c].push_back(b);
		}
	}

	// BFS to check connectivity
	std::vector<bool> visited(vertCount, false);
	std::queue<size_t> q;
	q.push(0);
	visited[0] = true;
	size_t visitedCount = 1;

	while (!q.empty()) {
		size_t curr = q.front();
		q.pop();
		for (size_t neighbor : adjacency[curr]) {
			if (!visited[neighbor]) {
				visited[neighbor] = true;
				q.push(neighbor);
				++visitedCount;
			}
		}
	}

	return visitedCount == vertCount;
}

bool MeshletsForMesh::verifyMeshletAdjacency(bool doLog) const {
	if (doLog) Log("MeshletsForMesh::verifyMeshletAdjacency() called, meshlets count: " << meshlets.size() << endl);
	if (meshlets.empty()) return true; // no meshlets to verify
	// Check if all meshlets are connected
	for (const auto& meshlet : meshlets) {
		if (!meshlet.isTrianglesConnected()) {
			if (doLog) Log("ERROR: Meshlet triangles are not connected!" << endl);
			return false;
		}
		if (!meshlet.isVerticesConnected()) {
			if (doLog) Log("ERROR: Meshlet vertices are not connected!" << endl);
			return false;
		}
	}
	if (doLog) Log("All meshlets verified successfully." << endl);
    return true;
}