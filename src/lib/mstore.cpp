#include "mainheader.h"

using namespace std;
using namespace glm;
using namespace tinygltf;

void MStore::init() {
	gltf.init(engine);

	// initialize structures on GPU global mesh storage:
	auto* mem = engine->globalRendering.getCurrentGPUMemoryChunk();

	// set array sizes, these are guarded and user needs to adjust them if too small at runtime
	size_t maxMeshes = engine->getMaxMeshes() * 10; // max meshes is a user setting, roughly we need 10 meshes as each 'user mesh' has 10 LOD
    size_t maxModels = engine->getMaxObjects();
    size_t maxMovingModels = engine->getMaxMovingObjects();
	size_t maxMaterials = engine->getMaxCollections();

	engine->globalRendering.gpuMemory.defineBuffer(BufferType::MeshInfos, sizeof(GPUMeshInfo), maxMeshes);
	engine->globalRendering.gpuMemory.defineBuffer(BufferType::Models, sizeof(GPUModel), maxModels);
	engine->globalRendering.gpuMemory.defineBuffer(BufferType::ModelsMoving, sizeof(GPUModel), maxMovingModels);
	engine->globalRendering.gpuMemory.defineBuffer(BufferType::ModelsParam, sizeof(GPUModelParam), maxMovingModels);
	engine->globalRendering.gpuMemory.defineBuffer(BufferType::Materials, sizeof(GPUMaterial), maxMaterials);
	engine->globalRendering.gpuMemory.defineBuffer(BufferType::FrameParams, sizeof(GPUFrameParam), MAX_DYNAMIC_LIGHTS);
	engine->globalRendering.gpuMemory.allocateBuffers();

    // we need a 1 to 1 mapping of GPUMeshInfo to MeshInfoMetadata for CPU side operations, so we preallocate the vector to maxMeshes
    meshMetadata.resize(maxMeshes);
    sceneObjects.resize(maxModels);
    movingSceneObjects.resize(maxMovingModels);
}

MeshFile* MStore::checkMeshFile(std::string filename, std::string id)
{
	// search id
	auto it = meshFileIDs.find(id);
	if (it != meshFileIDs.end()) {
		int index = it->second;
		if (index >= 0 && index < meshFiles.size()) {
			return &meshFiles[index];
		}
	}
	// id not found - search filename
    for (MeshFile& mf : meshFiles) {
        if (mf.filename == filename) {
            return &mf;
        }
    }
	return nullptr;
}

MeshFile* MStore::loadMesh(std::string filename, MeshFlagsCollection flags)
{
    MeshFile* existingMeshFile = checkMeshFile(filename, "");
    if (existingMeshFile != nullptr) {
        return existingMeshFile;
    }
    std::string id = "meshfile_" + std::to_string(meshFiles.size());
    return loadMesh(filename, id, flags);
}

MeshFile* MStore::loadMesh(std::string filename, std::string id, MeshFlagsCollection flags)
{
	MeshFile* existingMeshFile = checkMeshFile(filename, id);
	if (existingMeshFile != nullptr) {
		return existingMeshFile;
	}
	vector<byte> file_buffer;
	auto path = loadFile(filename, file_buffer);
    
	auto meshNumStart = engine->globalRendering.gpuMemory.getElementCount(BufferType::MeshInfos);
	string fileOrPath = (path) ? path.value() : filename;
	gltf.load2((const unsigned char*)file_buffer.data(), (int)file_buffer.size(), fileOrPath);
	auto meshNumCount = engine->globalRendering.gpuMemory.getElementCount(BufferType::MeshInfos) - meshNumStart;
    Log("MStore::loadMesh: Loaded " << meshNumCount << " meshes from file: " << filename << "\n");
 
    MeshFile meshFile{};
    meshFile.id = id;
    meshFile.flags = flags;
	meshFile.filename = filename;

	for (int i = 0; i < meshNumCount; ++i) {
		auto meshInfo = getGPUMeshInfoInternal(meshNumStart + i);
        auto metadata = getMeshMetadata(meshNumStart + i);
		meshInfo->index = meshNumStart + i;
		MeshFileEntry entry{};
        entry.name = metadata->name;
        entry.meshIndex = meshNumStart + i;
        meshFile.meshes.push_back(entry);
		metadata->meshFileIndex = meshFiles.size();
    }
    meshFiles.push_back(meshFile);
    addMeshFileID(id, meshFiles.size() - 1);

#if defined(DEBUG)
	for (auto& m : meshFile.meshes) {
		if (!checkBoundingBoxPlausibility(m.meshIndex)) {
			Error("Bounding box plausibility check failed for mesh " + m.name);
		}
	}
#endif
	aquireMeshletData(&meshFile, flags.hasFlag(MeshFlags::MESHLET_GENERATE));

	// work on flags:
    for (MeshFileEntry & entry : meshFile.meshes) {
        GPUMeshInfo* meshInfo = const_cast<GPUMeshInfo*>(getGPUMeshInfo(entry.meshIndex));
        handleFlags(*meshInfo, flags);
		// debug test
		//meshInfo->meshletOffset = 0x42;
		//meshInfo->localIndexOffset = 0x43;
		//meshInfo->globalIndexOffset = 0x44;
		//meshInfo->vertexOffset = 0x45;
		//meshInfo->meshletCount = 1;
		//meshInfo->material = 2;
		//meshInfo->index = 3;
		//meshInfo->next = 4;
	}
    return &meshFiles.back();
}

void MStore::uploadMesh(GPUMeshInfo* mi)
{
	auto& gb = engine->globalRendering.gpuMemory;
    auto metadata = getMeshMetadata(mi->index);
	assert(metadata->vertices.size() > 0);
	assert(metadata->indices.size() > 0);

	// calc aligned buffer sizes
	size_t vertexBufferSize = GlobalRendering::minAlign(metadata->vertices.size() * sizeof(PBRShader::Vertex));
	size_t globalIndexBufferSize = GlobalRendering::minAlign(metadata->outGlobalIndexBuffer.size() * sizeof(metadata->outGlobalIndexBuffer[0]));
	size_t localIndexBufferSize = GlobalRendering::minAlign(metadata->outLocalIndexPrimitivesBuffer.size() * sizeof(metadata->outLocalIndexPrimitivesBuffer[0]));
	size_t meshletDescBufferSize = GlobalRendering::minAlign(metadata->outMeshletDesc.size() * sizeof(PBRShader::PackedMeshletDesc));

	// copy vertices even if there is no meshlet data
	mi->vertexOffset = gb.copyToGlobalBuffer(vertexBufferSize, metadata->vertices.data());

    // rest of gpu data only makes sense if meshlet data is present
    if (meshletDescBufferSize > 0) {
		mi->globalIndexOffset = gb.copyToGlobalBuffer(globalIndexBufferSize, metadata->outGlobalIndexBuffer.data());
		mi->localIndexOffset = gb.copyToGlobalBuffer(localIndexBufferSize, metadata->outLocalIndexPrimitivesBuffer.data());
		mi->meshletOffset = gb.copyToGlobalBuffer(meshletDescBufferSize, metadata->outMeshletDesc.data());
        mi->meshletCount = static_cast<uint32_t>(metadata->outMeshletDesc.size());
	}
}

void MStore::uploadAllMeshes()
{
    auto meshcount = engine->globalRendering.gpuMemory.getElementCount(BufferType::MeshInfos);
    for (int i = 0; i < meshcount; i++) {
        GPUMeshInfo* meshInfo = getGPUMeshInfo(i);
		uploadMesh(meshInfo);
    }
}

GPUMeshInfo* MStore::getGPUMeshInfoInternal(int32_t index) {
	return engine->globalRendering.gpuMemory.getElementAddress<GPUMeshInfo>(BufferType::MeshInfos, index);
}

GPUMeshInfo* MStore::getGPUMeshInfo(int32_t index) {
	auto mi = getGPUMeshInfoInternal(index);
	if (!(mi != nullptr && mi->index == index)) {
		Error("Invalid GPUMeshInfo index: " + std::to_string(index));
	}
	return mi;
}

GPUMaterial* MStore::getGPUMaterial(int32_t index) {
	return engine->globalRendering.gpuMemory.getElementAddress<GPUMaterial>(BufferType::Materials, index);
}

GPUModel* MStore::getGPUModel(int32_t index) {
	return engine->globalRendering.gpuMemory.getElementAddress<GPUModel>(BufferType::Models, index);
}

int MStore::getUsedStationaryModelCount() const {
	return engine->globalRendering.gpuMemory.getElementCount(BufferType::Models);
}

int MStore::getUsedMovingModelCount() const {
	return engine->globalRendering.gpuMemory.getElementCount(BufferType::ModelsMoving);
}

GPUModel* MStore::getGPUMovingModel(int32_t index) {
	return engine->globalRendering.gpuMemory.getElementAddress<GPUModel>(BufferType::ModelsMoving, index);
}

GPUModelParam* MStore::getGPUModelParam(int32_t index) {
	return engine->globalRendering.gpuMemory.getElementAddress<GPUModelParam>(BufferType::ModelsParam, index);
}

GPUFrameParam* MStore::getGPUFrameParam(int32_t index) {
	auto count = engine->globalRendering.gpuMemory.getElementCount(BufferType::FrameParams);
	if (index < 0 || index >= count) {
		return nullptr;
	}
	return engine->globalRendering.gpuMemory.getElementAddress<GPUFrameParam>(BufferType::FrameParams, index);
}

SceneObject* MStore::getSceneObject(int32_t index) {
	return &sceneObjects[index];
}

SceneObject* MStore::getMovingSceneObject(int32_t index) {
	return &movingSceneObjects[index];
}

MeshInfoMetadata* MStore::getMeshMetadata(int32_t index) {
	if (index >= 0 && index < meshMetadata.size()) {
		return &meshMetadata[index];
	}
	return nullptr;
}

std::optional<std::string> MStore::loadFile(std::string filename, std::vector<std::byte>& fileBuffer)
{
	// find texture file, look in pak file first:
	PakEntry* pakFileEntry = nullptr;
	pakFileEntry = engine->files.findFileInPak(filename.c_str());
	// try file system if not found in pak:
	string binFile;
	if (pakFileEntry == nullptr) {
		binFile = engine->files.findFile(filename.c_str(), FileCategory::MESH);
		engine->files.readFile(binFile, fileBuffer, FileCategory::MESH);
		return binFile;
	}
	else {
		engine->files.readFile(pakFileEntry, fileBuffer, FileCategory::MESH);
		return std::nullopt;
	}
}

void MStore::addToGlobalBuffers(const std::vector<GPUMeshInfo>& gpuMeshInfos, const std::vector<MeshInfoMetadata>& gpuMeshMetadata,
	                            const std::vector<GPUMaterial>& gpuMaterialInfos, const std::vector<MaterialMetadata>& gpuMaterialMetadata)
{
	// Materials:
    auto globalMaterialStart = engine->globalRendering.gpuMemory.getElementCount(BufferType::Materials);

	for (int i = 0; i < gpuMaterialInfos.size(); i++) {
		GPUMaterial material = gpuMaterialInfos[i];
		MaterialMetadata materialMeta = gpuMaterialMetadata[i];
        GPUMaterial globalMaterial = material;

		if (globalMaterial.baseColorTextureSet >= 0) {
			globalMaterial.baseColorTextureSet = engine->mstore.gltf.getGlobalTextureIndex(static_cast<int>(material.baseColorTextureSet));
			TextureInfo* ti = engine->textureStore.getTextureByIndex(globalMaterial.baseColorTextureSet);
            ti->type = TextureType::TEXTURE_TYPE_GLTF;
		}
		if (globalMaterial.emissiveTextureSet >= 0) {
			globalMaterial.emissiveTextureSet = engine->mstore.gltf.getGlobalTextureIndex(static_cast<int>(material.emissiveTextureSet));
			TextureInfo* ti = engine->textureStore.getTextureByIndex(globalMaterial.emissiveTextureSet);
            ti->type = TextureType::TEXTURE_TYPE_GLTF;
		}
        if (globalMaterial.physicalDescriptorTextureSet >= 0) {
			globalMaterial.physicalDescriptorTextureSet = engine->mstore.gltf.getGlobalTextureIndex(static_cast<int>(material.physicalDescriptorTextureSet));
			TextureInfo* ti = engine->textureStore.getTextureByIndex(globalMaterial.physicalDescriptorTextureSet);
            ti->type = TextureType::TEXTURE_TYPE_GLTF;
		}
        if (globalMaterial.normalTextureSet >= 0) {
			globalMaterial.normalTextureSet = engine->mstore.gltf.getGlobalTextureIndex(static_cast<int>(material.normalTextureSet));
			TextureInfo* ti = engine->textureStore.getTextureByIndex(globalMaterial.normalTextureSet);
            ti->type = TextureType::TEXTURE_TYPE_GLTF;
		}
        if (globalMaterial.occlusionTextureSet >= 0) {
			globalMaterial.occlusionTextureSet = engine->mstore.gltf.getGlobalTextureIndex(static_cast<int>(material.occlusionTextureSet));
			TextureInfo* ti = engine->textureStore.getTextureByIndex(globalMaterial.occlusionTextureSet);
            ti->type = TextureType::TEXTURE_TYPE_GLTF;
		}

        // set the samplers for each texture. We must take care that for reused textures the sampler is the same as for the first usage.
		engine->textureStore.setAndCheckSampler(globalMaterial.baseColorTextureSet, materialMeta.samplerBaseColor);
		engine->textureStore.setAndCheckSampler(globalMaterial.emissiveTextureSet, materialMeta.samplerEmissive);
		engine->textureStore.setAndCheckSampler(globalMaterial.physicalDescriptorTextureSet, materialMeta.samplerMetallicRoughness);
		engine->textureStore.setAndCheckSampler(globalMaterial.normalTextureSet, materialMeta.samplerNormal);
		engine->textureStore.setAndCheckSampler(globalMaterial.occlusionTextureSet, materialMeta.samplerOcclusion);

		engine->globalRendering.gpuMemory.appendElement(BufferType::Materials, globalMaterial);
	}

    // Meshes:
    auto globalMeshStart = engine->globalRendering.gpuMemory.getElementCount(BufferType::MeshInfos);

    int i = 0;
    for (const auto& meshInfo : gpuMeshInfos) {
        GPUMeshInfo globalMeshInfo = meshInfo;
        globalMeshInfo.material = globalMaterialStart + meshInfo.material; // convert local material index to global material index
        globalMeshInfo.index = globalMeshStart + i; // convert local mesh index to global mesh index
        globalMeshInfo.next = (meshInfo.next > 0) ? globalMeshStart + meshInfo.next : 0; // convert local next index to global next index
        engine->globalRendering.gpuMemory.appendElement(BufferType::MeshInfos, globalMeshInfo);
		meshMetadata[globalMeshStart + i] = gpuMeshMetadata[i];
        i++;
    }
}

void MStore::handleFlags(GPUMeshInfo& mesh, MeshFlagsCollection flags)
{
    auto metadata = getMeshMetadata(mesh.index);
	if (flags.hasFlag(MeshFlags::MESH_TYPE_FLIP_WINDING_ORDER)) {
		// Flip winding order
		for (size_t i = 0; i < metadata->indices.size(); i += 3) {
			std::swap(metadata->indices[i], metadata->indices[i + 2]);
		}
	}

}

SceneObject* MStore::addObject(int32_t mesh_index, glm::vec3 pos, MeshFlagsCollection flags) {
	auto maxMeshNumber = engine->globalRendering.gpuMemory.getElementCount(BufferType::MeshInfos) - 1;
	if (mesh_index < 0 || mesh_index > maxMeshNumber) {
		Error("MStore::addObject: Invalid mesh index");
		return nullptr; // keep compiler happy
	}
	if (flags.hasFlag(MeshFlags::RENDER_TYPE_MOVING)) {
		// current index:
		auto objectIndex = engine->globalRendering.gpuMemory.getElementCount(BufferType::ModelsMoving);
		if (objectIndex >= movingSceneObjects.size()) Error("MStore::addObject: Exceeded maximum number of moving objects");

		GPUModel* model = getGPUMovingModel(objectIndex);
		GPUModelParam* modelParam = getGPUModelParam(objectIndex);
		modelParam->pos.x = 0.5f;
        modelParam->rot.y = 2.0f;
        modelParam->scale.z = 1.0f;
		SceneObject* obj = getMovingSceneObject(objectIndex);
		obj->index = objectIndex;
		obj->pos = pos;
		model->meshNumber = mesh_index;
		engine->globalRendering.gpuMemory.appendElement(BufferType::ModelsMoving, *model);
		engine->globalRendering.gpuMemory.appendElement(BufferType::ModelsParam, *modelParam);
		return obj;
	} else {
		// current index:
		auto objectIndex = engine->globalRendering.gpuMemory.getElementCount(BufferType::Models);
		if (objectIndex >= sceneObjects.size()) Error("MStore::addObject: Exceeded maximum number of stationary objects");

		GPUModel* model = getGPUModel(objectIndex);
		SceneObject* obj = getSceneObject(objectIndex);
		obj->index = objectIndex;
		obj->pos = pos;
		model->meshNumber = mesh_index;
		engine->globalRendering.gpuMemory.appendElement(BufferType::Models, *model);
		return obj;
	}
}

// Util methods
void MStore::getBoundingBox(BoundingBox& box, GPUMeshInfo& meshInfo)
{
    auto meta = getMeshMetadata(meshInfo.index);

	if (meta->boundingBoxAlreadySet) {
		box = meshInfo.boundingBox;
		return;
	}
	// iterate through vertices and find min/max:
	for (auto& v : meta->vertices) {
		if (v.pos.x < box.min.x) box.min.x = v.pos.x;
		if (v.pos.y < box.min.y) box.min.y = v.pos.y;
		if (v.pos.z < box.min.z) box.min.z = v.pos.z;
		if (v.pos.x > box.max.x) box.max.x = v.pos.x;
		if (v.pos.y > box.max.y) box.max.y = v.pos.y;
		if (v.pos.z > box.max.z) box.max.z = v.pos.z;
	}
	meshInfo.boundingBox = box;
	meta->boundingBoxAlreadySet = true;
}

bool MStore::checkBoundingBoxPlausibility(int32_t meshIndex)
{
    GPUMeshInfo* mi = getGPUMeshInfo(meshIndex);
	getBoundingBox(mi->boundingBox, *mi);
	string id = std::to_string(meshIndex);
	//Log("Bounding box for mesh " << id << ": Min(" << mi->boundingBox.min.x << ", " << mi->boundingBox.min.y << ", " << mi->boundingBox.min.z << "), Max(" << mi->boundingBox.max.x << ", " << mi->boundingBox.max.y << ", " << mi->boundingBox.max.z << ")\n");
	// check positive size:
	vec3 size = mi->boundingBox.max - mi->boundingBox.min;
	bool ret = true;
	if (size.x < 0 || size.y < 0 || size.z < 0) {
		Log("ERROR: Inverted bounding box for mesh " << id << endl);
		ret = false;
	}
    // anything below 1 mm is suspicious, but allow for flat objects (e.g., a plane)
	if (size.x < 0.001f || size.y < 0.001f || size.z < 0.001f) {
        // allow for flat objects, but log a warning
		if (length(size) > 0.001f && (size.x == 0 || size.y == 0 || size.z == 0)) {
			Log("WARNING: Flat bounding box for mesh " << id << ": Size(" << size.x << ", " << size.y << ", " << size.z << ")\n");
		} else {
			Log("ERROR: Very small bounding box for mesh " << id << ": Size(" << size.x << ", " << size.y << ", " << size.z << ")\n");
			ret = false;
		}
	}
	// anything above 10 km is suspicious:
	if (size.x > 20000.0f || size.y > 20000.0f || size.z > 20000.0f) {
		Log("ERROR: Very large bounding box for mesh " << id << ": Size(" << size.x << ", " << size.y << ", " << size.z << ")\n");
		ret = false;
	}
	if (ret == false) {
		Log("    bounding box error may mean vertices are off. This is the first triangle:\n");
		// get vertices for first triangle:
		logTriangleFromGlTF(0, mi);

	}
	return ret;
}

void MStore::logVertex(const PBRShader::Vertex& v)
{
	Log("Vertex: pos: " << v.pos.x << " " << v.pos.y << " " << v.pos.z
		<< ", normal: " << v.normal.x << " " << v.normal.y << " " << v.normal.z
		<< ", color: " << v.color.x << " " << v.color.y << " " << v.color.z
		<< ", uv: " << v.uv0.x << " " << v.uv0.y
		<< endl);
}

void MStore::logTriangleFromGlTF(int num, GPUMeshInfo* mesh)
{
    auto meta = getMeshMetadata(mesh->index);
	Log("Triangle " << num << ":" << endl);
	for (int i = 0; i < 3; ++i) {
		auto& v = meta->vertices[meta->indices[num * 3 + i]];
		Log("  Vertex " << i << " "); logVertex(v);
	}
}

void MStore::checkVertexDuplication(GPUMeshInfo* mesh)
{
    auto meta = getMeshMetadata(mesh->index);
	std::unordered_set<PBRShader::Vertex> uniqueVertices;
	for (auto& v : meta->vertices) {
		if (!uniqueVertices.insert(v).second) {
			// duplicate found, log v:
			//Log("duplicate vertex: "); logVertex(v);
			//logVertexIndex(v, mesh->vertices);
		}
	}
	size_t numDuplicates = meta->vertices.size() - uniqueVertices.size();
	if (numDuplicates > 0) {
		float percentage = (float)numDuplicates / (float)meta->vertices.size() * 100.0f;
		Log("WARNING: Mesh " << meta->name << " has duplicated vertices: " << numDuplicates << " (" << std::round(percentage) << "%) - you should consider cleaning up the mesh before usage." << endl);
	}
}

void MStore::aquireMeshletData(MeshFile* mfile, bool regenerateMeshletData)
{
	bool loadedFromFile = loadMeshletStorageFile(mfile);
	if (loadedFromFile) {
		return;
	}

	// not loaded - we have to regenerate meshlet data
	if (!regenerateMeshletData) {
		Log("ERROR: no meshlet file for mesh and regenerate not set - mesh is unusable: " << mfile->id << endl);
		return;
	}

	uint32_t meshletFlags = (uint32_t)MeshletFlags::MESHLET_ALG_GREEDY_DISTANCE; // | (uint32_t)MeshletFlags::MESHLET_SORT;

    // generate meshlet data for each mesh in the file:
	for (MeshFileEntry& entry : mfile->meshes) {
        auto meshInfo = getGPUMeshInfo(entry.meshIndex);
		calculateMeshlets(meshInfo, meshletFlags, GLEXT_MESHLET_VERTEX_COUNT, GLEXT_MESHLET_PRIMITIVE_COUNT - 1);
	}
}

bool MStore::loadMeshletStorageFile(MeshFile* mfile)
{
	return false;
}

void MStore::calculateMeshlets(GPUMeshInfo* mesh, uint32_t meshlet_flags, uint32_t vertexLimit, uint32_t primitiveLimit)
{
#   if defined(DEBUG)
	checkVertexDuplication(mesh);
#   endif
	assert(primitiveLimit < GLEXT_MESHLET_PRIMITIVE_COUNT); // we need one more primitive for adding the 'rest'
	assert(vertexLimit <= GLEXT_MESHLET_VERTEX_COUNT);

	//mesh->
	// min	[-0.040992 -0.046309 -0.053326]	glm::vec<3,float,0>
	// max	[0.040992 0.067943 0.132763]	glm::vec<3,float,0>
	BoundingBox box;
	getBoundingBox(box, *mesh);
	Log("bounding box min: " << box.min.x << " " << box.min.y << " " << box.min.z << endl);
	Log("bounding box max: " << box.max.x << " " << box.max.y << " " << box.max.z << endl);

	if (meshlet_flags & static_cast<uint32_t>(MeshletFlags::MESHLET_SORT)) {
		Log("WARNING: MESHLET_SORT was specified, but pre-sorting vertices is no longer available" << endl);
	}

	auto meta = getMeshMetadata(mesh->index);
	MeshletIn in{ meta->vertices, meta->indices, primitiveLimit, vertexLimit, box };
	MeshletOut out{ meta->meshletsForMesh.meshlets, meta->outMeshletDesc, meta->outLocalIndexPrimitivesBuffer, meta->outGlobalIndexBuffer };
	meta->meshletsForMesh.calculateTrianglesAndNeighbours(in);


	if (meshlet_flags & static_cast<uint32_t>(MeshletFlags::MESHLET_ALG_SIMPLE)) {
		meta->meshletsForMesh.applyMeshletAlgorithmSimple(in, out);
	}
	else if (meshlet_flags & static_cast<uint32_t>(MeshletFlags::MESHLET_ALG_GREEDY_VERT)) {
		meta->meshletsForMesh.applyMeshletAlgorithmGreedy(in, out, true);
	}
	else if (meshlet_flags & static_cast<uint32_t>(MeshletFlags::MESHLET_ALG_GREEDY_DISTANCE)) {
		meta->meshletsForMesh.applyMeshletAlgorithmGreedyDistance(in, out);
	}
	else {
		Log("WARNING: No meshlet algorithm specified, using greedy algorithm by default." << endl);
		meta->meshletsForMesh.applyMeshletAlgorithmGreedy(in, out, true);
	}
	// testing generated meshlets:
	meta->meshletsForMesh.verifyMeshletCoverage(true);
	meta->meshletsForMesh.verifyMeshletAdjacency(true);
    auto flags = meshFiles[meta->meshFileIndex].flags;
	if (flags.hasFlag(MeshFlags::MESHLET_DEBUG_COLORS)) {
		applyDebugMeshletColorsToVertices(mesh);
		applyDebugMeshletColorsToMeshlets(mesh);
	}
	meta->meshletsForMesh.fillMeshletOutputBuffers(in, out);
	logMeshletStats(mesh);
}

void MStore::applyDebugMeshletColorsToVertices(GPUMeshInfo* mesh)
{
	auto meta = getMeshMetadata(mesh->index);
	// color the meshlets:
	int meshletCount = 0;
	static auto col = engine->util.generateColorPalette256();
	for (auto& m : meta->meshletsForMesh.meshlets) {
		auto color = col[meshletCount % 256]; // assign color from palette
		meshletCount++;
		for (auto& v : m.vertices) {
			meta->vertices[v->globalIndex].color = color; // assign color to vertices in meshlet
		}
	}
}

void MStore::applyDebugMeshletColorsToMeshlets(GPUMeshInfo* mesh)
{
	auto meta = getMeshMetadata(mesh->index);
	for (auto& m : meta->meshletsForMesh.meshlets) {
		m.debugColors = true; // mark meshlet as having debug colors
	}
}

void MStore::logMeshletStats(GPUMeshInfo* mesh)
{
	auto meta = getMeshMetadata(mesh->index);
	assert(meta->meshletsForMesh.meshlets.size() > 0);
	Log("Meshlet stats for mesh " << meta->name << endl);
	Log("  Meshlets: " << meta->meshletsForMesh.meshlets.size() << endl);
	//Log("  MeshletOld descriptors: " << meta->outMeshletDesc.size() << endl);
	//Log("  MeshletOld triangles: " << mesh->meshlets.size() * 12 << endl); // each meshlet has 12 triangles
	int localIndexCount = 0; // count indices used for all meshlets
	int avgVertsPerMeshlet = 0;
	int avgPrimsPerMeshlet = 0;

	for (auto& m : meta->meshletsForMesh.meshlets) {
		localIndexCount += m.verticesIndices.size();
		avgPrimsPerMeshlet += m.triangles.size();
		avgVertsPerMeshlet += m.vertices.size();
		assert(m.vertices.size() <= 256); // we limit the number of vertices per meshlet to 256
	}
	avgPrimsPerMeshlet /= meta->meshletsForMesh.meshlets.size();
	avgVertsPerMeshlet /= meta->meshletsForMesh.meshlets.size();
	Log("  Average Meshlet verts / triangles: " << avgVertsPerMeshlet << " / " << avgPrimsPerMeshlet << endl);
	Log("  local Vertex indices (b4 greedy alg): " << meta->meshletsForMesh.indexVertexMap.size() << endl);
	Log("  local Vertex indices needed         : " << localIndexCount << endl);
	Log("  vertices: " << meta->meshletsForMesh.globalVertices.size() << endl);
}

void SceneObject::prepareGPUModel(GPUModel* gpuModel, glm::mat4& baseTransform)
{
	glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 rotationY = glm::rotate(glm::mat4(1.0f), rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 rotationZ = glm::rotate(glm::mat4(1.0f), rot.z, glm::vec3(0.0f, 0.0f, 1.0f));

	glm::mat4 rotationMatrix = rotationZ * rotationY * rotationX;
	glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, pos.z));
	glm::mat4 scaled = glm::scale(glm::mat4(1.0f), scale);

	// Apply baseTransform first (rightmost), then Scale, then Rotate, then Translate:
	gpuModel->model = trans * rotationMatrix * scaled * baseTransform;

	// convert flags to GPUModel.flags:
	if (flags.hasFlag(MeshFlags::MESH_TYPE_NO_TEXTURES)) {
		gpuModel->flags |= PBRShader::MODEL_RENDER_FLAG_USE_VERTEX_COLORS; // no textures, use vertex colors
	}
	if (flags.hasFlag(MeshFlags::MESHLET_DEBUG_COLORS)) {
		gpuModel->flags |= PBRShader::MODEL_RENDER_FLAG_USE_VERTEX_COLORS; // no textures, use vertex colors
	}
	if (flags.hasFlag(MeshFlags::RENDER_DISABLE)) {
		gpuModel->flags |= PBRShader::MODEL_RENDER_FLAG_DISABLE;
	}
}

void MStore::getFileInfosForMesh(int meshIndex, MeshFile& meshFile, MeshFileEntry& meshFileEntry)
{
    // we have no backlink from meshIndex to meshFile, so we have to iterate through all meshFiles and their entries
    for (auto& mf : meshFiles) {
        for (auto& entry : mf.meshes) {
            if (entry.meshIndex == meshIndex) {
                meshFile = mf;
                meshFileEntry = entry;
                return;
            }
        }
    }
    Error("MStore::getFileInfosForMesh: Mesh index not found: " + std::to_string(meshIndex));
}