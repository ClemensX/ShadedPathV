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
	if (!(meshes.size() < engine->getMaxMeshes())) {
        Error("MeshStore: too many meshes, increase max meshes in engine settings.");
	}
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
	if (coll->meshInfos.size() == 0) {
		Error("No meshes found in glTF file " + filename);
    }
	bool regenerate = flags.hasFlag(MeshFlags::MESHLET_GENERATE);
	for (auto mi : coll->meshInfos) {
		aquireMeshletData(filename, mi->id, regenerate);
	}
}

MeshCollection* MeshStore::getMeshCollection(std::string id)
{
	for (auto& coll : meshCollections) {
		if (coll.id == id) {
			return &coll;
		}
    }
	return nullptr;
}

void MeshStore::aquireMeshletData(std::string filename, std::string id, bool regenerateMeshletData)
{
	bool loadedFromFile = engine->meshStore.loadMeshletStorageFile(id, filename);
	if (loadedFromFile) return;

	// not loaded - we have to regenerate meshlet data
	if (!regenerateMeshletData) {
		Log("ERROR: no meshlet file for mesh and regenerate not set - mesh is unusable: " << id << endl);
		return;
	}

	uint32_t meshletFlags =
		(uint32_t)MeshletFlags::MESHLET_ALG_GREEDY_DISTANCE; // | (uint32_t)MeshletFlags::MESHLET_SORT;
	calculateMeshlets(id, meshletFlags, GLEXT_MESHLET_VERTEX_COUNT, GLEXT_MESHLET_PRIMITIVE_COUNT - 1);
}

void MeshStore::uploadObject(MeshInfo* obj)
{
	assert(obj->vertices.size() > 0);
	assert(obj->indices.size() > 0);

	// upload vec3 vertex buffer:
	size_t vertexBufferSize = GlobalRendering::minAlign(obj->vertices.size() * sizeof(PBRShader::Vertex));
	size_t globalIndexBufferSize = GlobalRendering::minAlign(obj->outGlobalIndexBuffer.size() * sizeof(obj->outGlobalIndexBuffer[0]));
	size_t localIndexBufferSize = GlobalRendering::minAlign(obj->outLocalIndexPrimitivesBuffer.size() * sizeof(obj->outLocalIndexPrimitivesBuffer[0]));
	size_t meshletDescBufferSize = GlobalRendering::minAlign(obj->outMeshletDesc.size() * sizeof(PBRShader::PackedMeshletDesc));

	if (meshletDescBufferSize > 0) {
        // global storage buffer:
		obj->GPUMeshStorageBaseAddress = engine->shaders.pbrShader.meshStorageBufferDeviceAddress;
		uint64_t pos = engine->globalRendering.uploadToGlobalBuffer(vertexBufferSize, obj->vertices.data(), engine->shaders.pbrShader.meshStorageBuffer);
		obj->vertexOffset = pos;

		pos = engine->globalRendering.uploadToGlobalBuffer(globalIndexBufferSize, obj->outGlobalIndexBuffer.data(), engine->shaders.pbrShader.meshStorageBuffer);
		obj->globalIndexOffset = pos;

		pos = engine->globalRendering.uploadToGlobalBuffer(localIndexBufferSize, obj->outLocalIndexPrimitivesBuffer.data(), engine->shaders.pbrShader.meshStorageBuffer);
		obj->localIndexOffset = pos;

		pos = engine->globalRendering.uploadToGlobalBuffer(meshletDescBufferSize, obj->outMeshletDesc.data(), engine->shaders.pbrShader.meshStorageBuffer);
		obj->meshletOffset = pos;
	}
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
                obj->available = false;
            }
		}
	}
	for (auto& mapobj : meshes) {
		if (mapobj.second.available) {
			auto& obj = mapobj.second;
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
	if (!(w->objectNum < meshStore->engine->getMaxObjects())) {
		Error("WorldObjectStore: too many objects, increase max objects in engine settings.");
    }
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
    Util::calculateBoundingBox(modelToWorld, box, boundingBoxCorners);
	return;
}

void WorldObject::drawBoundingBox(std::vector<LineDef>& boxes, glm::mat4 modelToWorld, vec4 color)
{
	BoundingBox box;
	getBoundingBox(box);
	Util::drawBoundingBox(boxes, box, boundingBoxCorners, modelToWorld, color);
}

void MeshStore::applyDebugMeshletColorsToVertices(MeshInfo* mesh)
{
	// color the meshlets:
	int meshletCount = 0;
	static auto col = engine->util.generateColorPalette256();
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

//bool CompareTriangles(const MeshletOld::MeshletTriangle* t1, const MeshletOld::MeshletTriangle* t2, const int idx) {
//	return (t1->centroid[idx] < t2->centroid[idx]);
//}
//
//bool compareVerts(const MeshletOld::MeshletVertInfo* v1, const MeshletOld::MeshletVertInfo* v2, const PBRShader::Vertex* vertexBuffer, const int idx) {
//	return (vertexBuffer[v1->index].pos[idx] < vertexBuffer[v2->index].pos[idx]);
//}

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

void MeshletsForMesh::calculateTrianglesAndNeighbours(MeshletIn& in)
{
	// Neighbour relation:
    // vertices: all triangles that use this vertex are stored in neighbourTriangles
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

void MeshletsForMesh::applyMeshletAlgorithmSimple(MeshletIn& in, MeshletOut& out, int numTrianglesPerMeshlet)
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
}

void MeshletsForMesh::applyMeshletAlgorithmGreedy(MeshletIn& in, MeshletOut& out, bool squeeze, bool useNearestNeighbour)
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
			auto& neighbours = curVertex->neighbourTriangles;
			if (useNearestNeighbour) {
				// sort neighbours by distance to current vertex
				sortNeighboursByDistance(in, curVertex, neighbours);
            }
			for (auto triangleIndex : neighbours) {
				Log("Processing triangle " << triangleIndex << " for vertex " << curVertex->globalIndex << std::endl);
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

void insertTriangle(MeshletsForMesh& m4m, Meshlet& m, uint32_t triIndex) {
	GlobalMeshletTriangle& tri = m4m.globalTriangles[triIndex];
	tri.usedInMeshlet = true;
	m.insertTriangle(tri);
	// recalc center
	m.center = vec3(0.0f);
	for (auto& t : m.triangles) {
		m.center += t.globalTriangle->centroid;
	}
	m.center /= static_cast<float>(m.triangles.size());
}

void addTriangle(MeshletsForMesh& m4m, MeshletOut& out, Meshlet& m, uint32_t triIndex) {
	GlobalMeshletTriangle& tri = m4m.globalTriangles[triIndex];
	if (m.canInsertTriangle(tri)) {
		insertTriangle(m4m, m, triIndex);
	} else {
		//finished = true; // no more triangles to insert
		out.meshlets.push_back(m);
		m.reset();
		insertTriangle(m4m, m, triIndex);
	}
}

uint32_t findNextTriangle(MeshletsForMesh& m4m, Meshlet& m, bool& finished) {
	vector<uint32_t> borderTriangleIndices; // indices into this->globalTriangles
	m4m.calcMeshletBorder(borderTriangleIndices, m);
    float bestDistance = std::numeric_limits<float>::max();
    finished = true; // assume we are done
	uint32_t nextTriangleIndex = 0;
	for (auto borderTriangleIndex : borderTriangleIndices) {
		auto& borderTriangle = m4m.globalTriangles[borderTriangleIndex];
		if (borderTriangle.usedInMeshlet) continue; // already in meshlet
		// calculate distance from current meshlet center to triangle centroid
		float distance = glm::length(m.center - borderTriangle.centroid);
		if (distance < bestDistance) {
			bestDistance = distance;
			nextTriangleIndex = borderTriangleIndex; // found a better triangle
            finished = false; // we found a triangle to insert
		}
    }
    return nextTriangleIndex;
}

// find nearest unfinished vertex to meshlet center
GlobalMeshletVertex* findNextVertexNearestToMeshletCenter(std::unordered_map<uint32_t, GlobalMeshletVertex*>& verticesMap, MeshletsForMesh& m4m, Meshlet& m, const MeshletIn& in) {
	// return first vertex on first call:
	if (verticesMap.size() == m4m.globalVertices.size()) {
		return &m4m.globalVertices[0];
	}
	std::vector<uint32_t> borderVerticesIndices; // indices into this->globalVertices
	m4m.calcMeshletBorder(verticesMap, borderVerticesIndices, m);
	// find nearest vertex to current meshlet center:
	float bestDistance = std::numeric_limits<float>::max();
	GlobalMeshletVertex* nextVertex = nullptr;
	for (auto borderVertexIndex : borderVerticesIndices) {
		GlobalMeshletVertex* borderVertex = &m4m.globalVertices[borderVertexIndex];
		// calculate distance from current meshlet center to vertex position
		float distance = glm::length(m.center - in.vertices[borderVertexIndex].pos);
		if (distance < bestDistance) {
			bestDistance = distance;
			nextVertex = borderVertex; // found a better vertex
		}
	}
	return nextVertex;
}

// find nearest unfinished vertex relative to first meshlet vertex
GlobalMeshletVertex* findNextVertexNearestToMeshletStart(std::unordered_map<uint32_t, GlobalMeshletVertex*>& verticesMap, MeshletsForMesh& m4m, Meshlet& m, const MeshletIn& in) {
	// return first vertex on first call:
	if (verticesMap.size() == m4m.globalVertices.size()) {
		return &m4m.globalVertices[0];
	}
	if (m.vertices.size() == 0) {
		Log("ERROR: findNextVertexNearestToMeshletStart called with empty meshlet!" << std::endl);
	}
	vec3 startPos = in.vertices[m.vertices[0]->globalIndex].pos;
	std::vector<uint32_t> borderVerticesIndices; // indices into this->globalVertices
	m4m.calcMeshletBorder(verticesMap, borderVerticesIndices, m);
	// find nearest vertex to current meshlet center:
	float bestDistance = std::numeric_limits<float>::max();
	GlobalMeshletVertex* nextVertex = nullptr;
	for (auto borderVertexIndex : borderVerticesIndices) {
		GlobalMeshletVertex* borderVertex = &m4m.globalVertices[borderVertexIndex];
		// calculate distance from current meshlet center to vertex position
		float distance = glm::length(startPos - in.vertices[borderVertexIndex].pos);
		if (distance < bestDistance) {
			bestDistance = distance;
			nextVertex = borderVertex; // found a better vertex
		}
	}
	return nextVertex;
}

// find nearest unfinished vertex relative to first meshlet vertex, not using border
GlobalMeshletVertex* findNextVertexNearestToMeshletStartNoBorder(std::unordered_map<uint32_t, GlobalMeshletVertex*>& verticesMap, MeshletsForMesh& m4m, Meshlet& m, const MeshletIn& in) {
	// return first vertex on first call:
	if (verticesMap.size() == m4m.globalVertices.size()) {
		return &m4m.globalVertices[0];
	}
	if (m.vertices.size() == 0) {
		Log("ERROR: findNextVertexNearestToMeshletStart called with empty meshlet!" << std::endl);
	}
	vec3 startPos = in.vertices[m.vertices[0]->globalIndex].pos;
	// find nearest vertex to current meshlet center:
	float bestDistance = std::numeric_limits<float>::max();
	GlobalMeshletVertex* nextVertex = nullptr;
	for (auto vertexMapEntry: verticesMap) {
        uint32_t unfinishedVertexIndex = vertexMapEntry.first;
		GlobalMeshletVertex* borderVertex = &m4m.globalVertices[unfinishedVertexIndex];
		// calculate distance from current meshlet center to vertex position
		float distance = glm::length(startPos - in.vertices[unfinishedVertexIndex].pos);
		if (distance < bestDistance) {
			bestDistance = distance;
			nextVertex = borderVertex; // found a better vertex
		}
	}
	return nextVertex;
}

void MeshletsForMesh::applyMeshletAlgorithmGreedyDistance(MeshletIn& in, MeshletOut& out)
{
	Log("Meshlet algorithm GREEDY DISTANCE started for " << in.vertices.size() << " vertices and " << in.indices.size() << " indices" << std::endl);
	/*
    * sketch of algorithm:
	* 
    * if not all vertices used: choose nearest unused vertex v
    *     put v to queue
    *     while queue not empty
    *         if v finished: continue
	*             trilist = sorted nearest triangles using v
    *         while trilist not empty
    *             choose nearest unfinished triangle t of v and add to meshlet
	*             add unfishied vertices of t to queue
    *             mark finished vertices
    *         
	*/
	std::unordered_map<uint32_t, GlobalMeshletVertex*> verticesMap;
	// fill vertices map with pointers to all vertices
	for (auto& v : globalVertices) {
		verticesMap[v.globalIndex] = &v;
	}
	std::vector<glm::vec3> positions;
	for (const auto& v : in.vertices) positions.push_back(v.pos);
	KDTree3D tree(positions);
	Meshlet m(this, in.primitiveLimit, in.vertexLimit);
	GlobalMeshletVertex* curVertex = nullptr;
	// start with first vertex:
	curVertex = &this->globalVertices[0];
	while (curVertex != nullptr) {
		if (verticesMap.size() % 10000 == 0) {
			Log("  applyMeshletAlgorithmGreedyDistance: still " << verticesMap.size() << " unfinished vertices." << std::endl);
		}
		//vec3 queryPos = in.vertices[curVertex->globalIndex].pos;
		for (auto triIndex : curVertex->neighbourTriangles) {
			if (globalTriangles[triIndex].usedInMeshlet) continue; // already in meshlet
			addTriangle(*this, out, m, triIndex);
		}
		// mark vertex as finished
		curVertex->usedInMeshlet = true;
		verticesMap.erase(curVertex->globalIndex);
		tree.markUsed(curVertex->globalIndex);

		// find next vertex at borders
		curVertex = findNextVertexNearestToMeshletStart(verticesMap, *this, m, in);
		if (curVertex != nullptr) continue; // found a border vertex

		// find next nearest unused vertex
		vec3 startPos = in.vertices[m.vertices[0]->globalIndex].pos;
		uint32_t idx = tree.nearestUnused(startPos);
		if (idx != std::numeric_limits<uint32_t>::max()) {
			// Use idx as the next vertex
			curVertex = &this->globalVertices[idx];
		}
		else {
			curVertex = nullptr; // we are done
		}
	}
	if (m.triangles.size() > 0) {
		out.meshlets.push_back(m);
	}
}

void MeshletsForMesh::sortNeighboursByDistance(MeshletIn& in, GlobalMeshletVertex* vertex, std::vector<uint32_t>& neighbours)
{
	std::sort(neighbours.begin(), neighbours.end(),
		[in,vertex,this](uint32_t a, uint32_t b) {
            //Log("Sorting neighbours by distance from vertex: " << vertex->globalIndex << a << " vs " << b << std::endl);
            auto vIndex = vertex->globalIndex;
            auto& v = in.vertices[vIndex];
			//MeshStore::logVertex(v);
            auto& a0pos = in.vertices[this->globalTriangles[a].indices[0]].pos;
            auto& a1pos = in.vertices[this->globalTriangles[a].indices[1]].pos;
            auto& a2pos = in.vertices[this->globalTriangles[a].indices[2]].pos;
			glm::vec3 centroidA = (a0pos + a1pos + a2pos) / 3.0f;
			auto& b0pos = in.vertices[this->globalTriangles[b].indices[0]].pos;
			auto& b1pos = in.vertices[this->globalTriangles[b].indices[1]].pos;
            auto& b2pos = in.vertices[this->globalTriangles[b].indices[2]].pos;
            glm::vec3 centroidB = (b0pos + b1pos + b2pos) / 3.0f;
            // calculate distance from vertex to triangle centroids
			float distanceA = glm::length(v.pos - centroidA);
            float distanceB = glm::length(v.pos - centroidB);

			return distanceA < distanceB;
		});
	Log("Sorted neighbours for vertex " << vertex->globalIndex << ": " << endl);
	for (auto n : neighbours) {
		uint32_t i0 = this->globalTriangles[n].indices[0];
		uint32_t i1 = this->globalTriangles[n].indices[1];
		uint32_t i2 = this->globalTriangles[n].indices[2];
		auto& a0pos = in.vertices[i0].pos;
		auto& a1pos = in.vertices[i1].pos;
		auto& a2pos = in.vertices[i2].pos;
		glm::vec3 centroidA = (a0pos + a1pos + a2pos) / 3.0f;
		float distanceA = glm::length(in.vertices[vertex->globalIndex].pos - centroidA);
		Log("tri " << i0 << " " << i1 << " " << i2 << " distance: " << distanceA << endl);
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

void MeshletsForMesh::calcMeshletBorder(std::unordered_map<uint32_t, GlobalMeshletVertex*>& verticesMap, std::vector<uint32_t>& borderVerticesIndices, Meshlet& m)
{
	borderVerticesIndices.clear();
	// iterate through all vertices inside the meshlet and add all unfinished vertices
	for (auto& v : m.vertices) {
		//Log("Vertex " << v->globalIndex << " has " << v->neighbourTriangles.size() << " neighbour triangles." << endl);
        if (verticesMap.find(v->globalIndex) != verticesMap.end()) { // TODO maybe using flag 'used_in_meshlet' is enough?
			// vertex is not yet finished
            borderVerticesIndices.push_back(v->globalIndex);
        }
	}
}

void MeshletsForMesh::generatePackedBoundingBoxData(MeshletIn& in, MeshletOut& out)
{
	// make sure we have calculated the objects AABB
    assert(in.boundingBox.min.x < std::numeric_limits<float>::max());
    if (out.meshlets.size() == 0) return;
    // simple test to find out if we need to calculate bounding boxes:
	if (out.meshlets[0].boundingBox.min.x == std::numeric_limits<float>::max()) {
		// bounding boxes already calculated
		Log("regenerating bounding box info for meshlet");
		for (size_t i = 0; i < out.meshlets.size(); ++i) {
			auto& m = out.meshlets[i];
			for (size_t j = 0; j < m.vertices.size(); ++j) {
				auto& v = in.vertices[m.vertices[j]->globalIndex];
				if (v.pos.x < m.boundingBox.min.x) m.boundingBox.min.x = v.pos.x;
				if (v.pos.y < m.boundingBox.min.y) m.boundingBox.min.y = v.pos.y;
				if (v.pos.z < m.boundingBox.min.z) m.boundingBox.min.z = v.pos.z;
				if (v.pos.x > m.boundingBox.max.x) m.boundingBox.max.x = v.pos.x;
				if (v.pos.y > m.boundingBox.max.y) m.boundingBox.max.y = v.pos.y;
                if (v.pos.z > m.boundingBox.max.z) m.boundingBox.max.z = v.pos.z;
			}
            m.packedBoundingBox = Util::packBoundingBox48(m.boundingBox, in.boundingBox);
		}
    }
}

void MeshletsForMesh::fillMeshletOutputBuffers(MeshletIn& in, MeshletOut& out)
{
    generatePackedBoundingBoxData(in, out);
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
	out.outLocalIndexPrimitivesBuffer.resize(GlobalRendering::minAlign(totalIndices * 3));
	uint32_t indexBufferOffset = 0;
	for (size_t i = 0; i < out.meshlets.size(); ++i) {
		auto& m = out.meshlets[i];
		uint8_t vp = 0; // default rendering mode
		if (m.debugColors) {
			vp = 0x01; // use debug colors
		}
		PBRShader::PackedMeshletDesc packed = PBRShader::PackedMeshletDesc::pack(m.packedBoundingBox, m.vertices.size(), m.triangles.size(), vp, indexBufferOffset, 0xABCDEF);
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

	if (meshlet_flags & static_cast<uint32_t>(MeshletFlags::MESHLET_SORT)) {
		Log("WARNING: MESHLET_SORT was specified, but pre-sorting vertices is no longer available" << endl);
	}

	MeshletIn in{ mesh->vertices, mesh->indices, primitiveLimit, vertexLimit, box };
	MeshletOut out{ mesh->meshletsForMesh.meshlets, mesh->outMeshletDesc, mesh->outLocalIndexPrimitivesBuffer, mesh->outGlobalIndexBuffer };
	mesh->meshletsForMesh.calculateTrianglesAndNeighbours(in);


	if (meshlet_flags & static_cast<uint32_t>(MeshletFlags::MESHLET_ALG_SIMPLE)) {
		mesh->meshletsForMesh.applyMeshletAlgorithmSimple(in, out);
	} else if (meshlet_flags & static_cast<uint32_t>(MeshletFlags::MESHLET_ALG_GREEDY_VERT)) {
		mesh->meshletsForMesh.applyMeshletAlgorithmGreedy(in, out, true);
	} else if (meshlet_flags & static_cast<uint32_t>(MeshletFlags::MESHLET_ALG_GREEDY_DISTANCE)) {
		mesh->meshletsForMesh.applyMeshletAlgorithmGreedyDistance(in, out);
	} else {
		Log("WARNING: No meshlet algorithm specified, using greedy algorithm by default." << endl);
		mesh->meshletsForMesh.applyMeshletAlgorithmGreedy(in, out, true);
    }
	// testing generated meshlets:
	mesh->meshletsForMesh.verifyMeshletCoverage(true);
	mesh->meshletsForMesh.verifyMeshletAdjacency(true);


	if (mesh->flags.hasFlag(MeshFlags::MESHLET_DEBUG_COLORS)) {
		applyDebugMeshletColorsToVertices(mesh);
		applyDebugMeshletColorsToMeshlets(mesh);
	}
	mesh->meshletsForMesh.fillMeshletOutputBuffers(in, out);
    logMeshletStats(mesh);
}

void MeshStore::logVertex(const PBRShader::Vertex& v)
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

void MeshStore::debugGraphics(WorldObject* obj, FrameResources& fr, glm::mat4 modelToWorld, bool drawBoundingBox, bool drawVertices, bool drawNormals, bool drawMeshletBoundingBoxes, glm::vec4 colorVertices, glm::vec4 colorNormal, glm::vec4 colorBoxes, float normalLineLength)
{
	// get access to line shader
	auto& lineShader = engine->shaders.lineShader;
	if (!lineShader.enabled) return;
	if (!obj->enableDebugGraphics) return;

	LineDef l;
	vector<LineDef> addLines;
	if (drawBoundingBox) {
		obj->drawBoundingBox(addLines, modelToWorld, colorNormal);
	}
    if (drawMeshletBoundingBoxes) {
		for (auto& m : obj->mesh->meshletsForMesh.meshlets) {
			BoundingBoxCorners bbcorners;
			//Util::drawBoundingBox(addLines, m.boundingBox, bbcorners, modelToWorld, colorBoxes);
		}
		if (obj->mesh->outMeshletDesc.size() > 0) {
			auto& packed = obj->mesh->outMeshletDesc[0];
			uint64_t bb = packed.getBoundingBox();
			//Log("Meshlet 0 aabb: " << std::hex << bb << std::dec << endl);
			BoundingBox meshletBB;
			BoundingBox objectBB;
			obj->mesh->getBoundingBox(objectBB);
			Util::unpackBoundingBox48(bb, meshletBB, objectBB);
            //Log("  min: " << meshletBB.min.x << " " << meshletBB.min.y << " " << meshletBB.min.z);
            //Log("  max: " << meshletBB.max.x << " " << meshletBB.max.y << " " << meshletBB.max.z << endl);
		}
		for (auto& packed : obj->mesh->outMeshletDesc) {
			BoundingBoxCorners bbcorners;
			uint64_t bb = packed.getBoundingBox();
			BoundingBox meshletBB;
			BoundingBox objectBB;
			obj->mesh->getBoundingBox(objectBB);
			Util::unpackBoundingBox48(bb, meshletBB, objectBB);
			Util::drawBoundingBox(addLines, meshletBB, bbcorners, modelToWorld, colorBoxes);
		}
    }
    // add vertices:
	if (drawVertices) {
		for (long i = 0; i < obj->mesh->indices.size(); i += 3) {
			l.color = colorVertices;
			auto& v0 = obj->mesh->vertices[obj->mesh->indices[i + 0]];
			auto& v1 = obj->mesh->vertices[obj->mesh->indices[i + 1]];
			auto& v2 = obj->mesh->vertices[obj->mesh->indices[i + 2]];
			vec3 p0 = vec3(modelToWorld * vec4(v0.pos, 1.0f));
			vec3 p1 = vec3(modelToWorld * vec4(v1.pos, 1.0f));
			vec3 p2 = vec3(modelToWorld * vec4(v2.pos, 1.0f));
			l.start = p0;
			l.end = p1;
			addLines.push_back(l);
			l.start = p1;
			l.end = p2;
			addLines.push_back(l);
			l.start = p2;
			l.end = p0;
			addLines.push_back(l);
		}
	}

	// add normals:
	if (drawNormals) {
		for (auto& v : obj->mesh->vertices) {
			l.color = colorNormal;
			l.start = vec3(modelToWorld * vec4(v.pos, 1.0f));
			l.end = l.start + v.normal * normalLineLength;
			addLines.push_back(l);
		}
	}
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
    localTriangle.globalTriangle = &triangle; // store pointer to global triangle for later reference
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
	aquireMeshletData(id, id);
}

void MeshStore::loadMeshCylinder(std::string id, MeshFlagsCollection flags, std::string baseColorTextureId, bool produceCrack, int segments, int heightDivs, float radius, float height)
{
	MeshInfo meshInfo;
	meshInfo.id = id;
	meshInfo.flags = flags;

	Util::GenerateCylinderMesh(segments, heightDivs, radius, height, meshInfo.vertices, meshInfo.indices, produceCrack);

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
	if (flags.hasFlag(MeshFlags::MESHLET_GENERATE)) {
		aquireMeshletData(id, id, true);
	} else {
		aquireMeshletData(id, id);
	}
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
			if (shared == 1) {
				//Log("shared 1\n");
			}
			if (shared >= 1) {
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

struct IndicesHash {
	size_t operator()(const std::array<uint32_t, 3>& arr) const noexcept {
		size_t h1 = std::hash<uint32_t>{}(arr[0]);
		size_t h2 = std::hash<uint32_t>{}(arr[1]);
		size_t h3 = std::hash<uint32_t>{}(arr[2]);
		size_t seed = h1;
		seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

using IndicesKey = std::array<uint32_t, 3>;

bool MeshletsForMesh::verifyMeshletCoverage(bool doLog) const
{
	std::unordered_map<IndicesKey, GlobalMeshletTriangle, IndicesHash> triangleMap;
	// Check if all triangles are used in meshlets

    vector<bool> verticesUsed(globalVertices.size(), false);
	bool allVerticesUsed = true;
	bool allTrianglesUsed = true;
	for (const auto& meshlet : meshlets) {
		for (const auto& tri : meshlet.triangles) {
            // get global indices of triangle:
			auto& v0 = meshlet.vertices[tri.indices[0]]->globalIndex;
			auto& v1 = meshlet.vertices[tri.indices[1]]->globalIndex;
			auto& v2 = meshlet.vertices[tri.indices[2]]->globalIndex;
            IndicesKey key = { v0, v1, v2 };
            for (auto vi : key) {
				verticesUsed[vi] = true; // mark vertex as used
            }
            auto it = triangleMap.find(key);
			if (it == triangleMap.end()) {
				triangleMap[key] = *tri.globalTriangle; // store triangle in map
			} else {
                // duplicate triangle found (may be problem of original mesh, so only a warning)
				if (doLog) Log("WARNING: Duplicate triangle found in meshlets: " << v0 << " " << v1 << " " << v2 << endl);
            }
		}
    }
    // Now check if all global triangles are in the map
	for (size_t i = 0; i < globalTriangles.size(); i++) {
		auto& tri = globalTriangles[i];
		IndicesKey key = { tri.indices[0], tri.indices[1], tri.indices[2] };
		if (triangleMap.find(key) == triangleMap.end()) {
			if (doLog) Log("ERROR: Triangle not covered by any meshlet: globalTriangles[" << i << "] " << tri.indices[0] << " " << tri.indices[1] << " " << tri.indices[2] << endl);
			allTrianglesUsed = false;
		}
    }
	for (size_t i = 0; i < verticesUsed.size(); i++) {
		if (!verticesUsed[i]) {
			if (doLog) Log("ERROR: Vertex not used in any meshlet: vertex index " << i << endl);
			allVerticesUsed = false;
		}
    }
	return false;
}

uint64_t MeshStore::getUsedStorageSize() {
	auto max = engine->getMeshStorageSize();
	auto used = engine->shaders.pbrShader.getMeshStorageBufferUsage();
	return used;
}

bool MeshStore::writeMeshletStorageFile(std::string id, string fileBaseName)
{
    MeshInfo* meshInfo = getMesh(id);
	// open meshlet file for write:
    // concatenate mesh name with .meshlet extension
    string fileName = fileBaseName + ".meshlet";
	string meshFile = engine->files.findFile(fileName, FileCategory::MESH, false, true);
	// open meshFile for writing:
	std::ofstream mf(meshFile, std::ios::binary);
	// write file type identifier:
	const char fileType[16] = "SPMESHLETFILEv1";
	mf.write(fileType, 16); // write file type string and trailing zero
	// write meshlet data:
    MeshletStorageData meshletData;
    meshletData.numMeshlets = meshInfo->outMeshletDesc.size();
    meshletData.numLocalIndices = meshInfo->outLocalIndexPrimitivesBuffer.size();
    meshletData.numGlobalIndices = meshInfo->outGlobalIndexBuffer.size();
	if (meshletData.numMeshlets == 0 || meshletData.numGlobalIndices == 0 || meshletData.numLocalIndices == 0) {
		Log("ERROR: Trying to write meshlet file for empty mesh " << id << endl);
		return false;
    }
    // write buffer lengths:
	mf.write((char*)&meshletData, sizeof(MeshletStorageData));
	// write meshlet descriptors:
    mf.write((char*)meshInfo->outMeshletDesc.data(), sizeof(PBRShader::PackedMeshletDesc) * meshletData.numMeshlets);
    // write local index buffer:
    mf.write((char*)meshInfo->outLocalIndexPrimitivesBuffer.data(), sizeof(uint8_t) * meshletData.numLocalIndices);
    // write global index buffer:
    mf.write((char*)meshInfo->outGlobalIndexBuffer.data(), sizeof(uint32_t) * meshletData.numGlobalIndices);
	// close file:
	mf.close();
	return true;
}

bool MeshStore::loadMeshletStorageFile(std::string id, string fileBaseName)
{
	MeshInfo* meshInfo = getMesh(id);
	// open meshlet file for read:
	// concatenate mesh name with .meshlet extension
	string fileName = fileBaseName + ".meshlet";
	string meshFile = engine->files.findFile(fileName, FileCategory::MESH, false, false);
	if (meshFile.empty()) {
		Log("ERROR: Meshlet file not found for mesh " << id << endl);
		return false;
    }
	vector<byte> file_buffer;
	engine->files.readFile(fileName, file_buffer, FileCategory::MESH);
	const char fileType[16] = "SPMESHLETFILEv1";
    // check file_buffer for correct header:
    if (file_buffer.size() < 16) {
		Log("ERROR: Meshlet file too small for mesh " << id << endl);
		return false;
    }
    if (memcmp(file_buffer.data(), fileType, 16) != 0) {
        Log("ERROR: Meshlet file has incorrect header for mesh " << id << endl);
        return false;
    }
	// read meshlet buffer sizes:
	MeshletStorageData meshletData;
    size_t offset = 16;
    if (file_buffer.size() < offset + sizeof(MeshletStorageData)) {
        Log("ERROR: Meshlet file too small for mesh " << id << endl);
        return false;
    }
    memcpy(&meshletData, file_buffer.data() + offset, sizeof(MeshletStorageData));
    offset += sizeof(MeshletStorageData);
	if (meshletData.numMeshlets == 0 || meshletData.numGlobalIndices == 0 || meshletData.numLocalIndices == 0) {
		Log("ERROR: Meshlet file has zero sizes for mesh " << id << endl);
		return false;
    }
	// resize mesh buffers:
	meshInfo->outMeshletDesc.resize(meshletData.numMeshlets);
	meshInfo->outLocalIndexPrimitivesBuffer.resize(meshletData.numLocalIndices);
	meshInfo->outGlobalIndexBuffer.resize(meshletData.numGlobalIndices);
	size_t expectedSize = offset + sizeof(PBRShader::PackedMeshletDesc) * meshletData.numMeshlets
		+ sizeof(uint8_t) * meshletData.numLocalIndices
		+ sizeof(uint32_t) * meshletData.numGlobalIndices;
	if (file_buffer.size() < expectedSize) {
		Log("ERROR: Meshlet file too small for mesh " << id << endl);
		return false;
    }
    // read meshlet descriptors:
    memcpy(meshInfo->outMeshletDesc.data(), file_buffer.data() + offset, sizeof(PBRShader::PackedMeshletDesc) * meshletData.numMeshlets);
    offset += sizeof(PBRShader::PackedMeshletDesc) * meshletData.numMeshlets;
    // read local index buffer:
    memcpy(meshInfo->outLocalIndexPrimitivesBuffer.data(), file_buffer.data() + offset, sizeof(uint8_t) * meshletData.numLocalIndices);
    offset += sizeof(uint8_t) * meshletData.numLocalIndices;
    // read global index buffer:
    memcpy(meshInfo->outGlobalIndexBuffer.data(), file_buffer.data() + offset, sizeof(uint32_t) * meshletData.numGlobalIndices);
    meshInfo->meshletStorageFileFound = true;
	return true;
}
