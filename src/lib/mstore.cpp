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
	engine->globalRendering.gpuMemory.defineBuffer(BufferType::Materials, sizeof(GPUMaterial), maxMaterials);
	engine->globalRendering.gpuMemory.allocateBuffers();

    // we need a 1 to 1 mapping of GPUMeshInfo to MeshInfoMetadata for CPU side operations, so we preallocate the vector to maxMeshes
    meshMetadata.resize(maxMeshes);
    sceneObjects.resize(maxModels);
    movingSceneObjects.resize(maxMovingModels);
}

void MStore::loadMesh(std::string filename, std::string id, MeshFlagsCollection flags)
{
    assert(engine != nullptr);
	vector<byte> file_buffer;
	auto path = loadFile(filename, file_buffer);
    
	// test
	//GPUMeshInfo meshInfo{};
 //   engine->globalRendering.gpuMemory.appendElement(BufferType::MeshInfos, meshInfo);
 //   size_t count = engine->globalRendering.gpuMemory.getElementCount(BufferType::MeshInfos);
	//assert(count > 0);
	//const GPUMeshInfo* meshInfoPtr = engine->globalRendering.gpuMemory.getCppBuffer<GPUMeshInfo>(BufferType::MeshInfos, 0);
	//assert(meshInfoPtr != nullptr);
	//assert(meshInfoPtr->globalIndexOffset == 0);
 //   assert(meshInfoPtr[0].localIndexOffset == 0);

	auto meshNumStart = engine->globalRendering.gpuMemory.getElementCount(BufferType::MeshInfos);
	string fileOrPath = (path) ? path.value() : filename;
	gltf.load2((const unsigned char*)file_buffer.data(), (int)file_buffer.size(), fileOrPath);
	auto meshNumCount = engine->globalRendering.gpuMemory.getElementCount(BufferType::MeshInfos) - meshNumStart;
    Log("MStore::loadMesh: Loaded " << meshNumCount << " meshes from file: " << filename << "\n");
 
    MeshFile meshFile{};
    meshFile.id = id;
    meshFile.flags = flags;
	for (int i = 0; i < meshNumCount; ++i) {
		auto meshInfo = getGPUMeshInfo(meshNumStart + i);
        auto metadata = getMeshMetadata(meshNumStart + i);
		MeshFileEntry entry{};
        entry.name = metadata->name;
        entry.meshIndex = meshNumStart + i;
        meshFile.meshes.push_back(entry);
    }
    meshFiles.push_back(meshFile);
    addMeshFileID(id, static_cast<int32_t>(meshFiles.size() - 1));

	// work on flags:
    for (MeshFileEntry & entry : meshFile.meshes) {
        GPUMeshInfo* meshInfo = const_cast<GPUMeshInfo*>(engine->globalRendering.gpuMemory.getCppBuffer<GPUMeshInfo>(BufferType::MeshInfos, entry.meshIndex));
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

}

void MStore::uploadMesh(GPUMeshInfo* mi)
{
	auto& gb = engine->globalRendering.gpuMemory;
    auto metadata = getMeshMetadata(mi->index);
	assert(metadata->vertices.size() > 0);
	assert(metadata->indices.size() > 0);

	// upload vec3 vertex buffer:
	size_t vertexBufferSize = GlobalRendering::minAlign(metadata->vertices.size() * sizeof(PBRShader::Vertex));
	uint64_t pos = gb.copyToGlobalBuffer(vertexBufferSize, metadata->vertices.data());
	mi->vertexOffset = pos;
}

void MStore::uploadAllMeshes()
{
    auto meshcount = engine->globalRendering.gpuMemory.getElementCount(BufferType::MeshInfos);
    for (int i = 0; i < meshcount; i++) {
        GPUMeshInfo* meshInfo = getGPUMeshInfo(i);
		uploadMesh(meshInfo);
    }
}

const GPUMeshInfo* MStore::getGPUMeshInfo(int32_t index) const {
	return engine->globalRendering.gpuMemory.getCppBuffer<GPUMeshInfo>(BufferType::MeshInfos, index);
}

GPUMeshInfo* MStore::getGPUMeshInfo(int32_t index) {
	return engine->globalRendering.gpuMemory.getCppBuffer<GPUMeshInfo>(BufferType::MeshInfos, index);
}

const GPUMaterial* MStore::getGPUMaterial(int32_t index) const {
	return engine->globalRendering.gpuMemory.getCppBuffer<GPUMaterial>(BufferType::Materials, index);
}

GPUModel* MStore::getGPUModel(int32_t index) {
	return engine->globalRendering.gpuMemory.getCppBuffer<GPUModel>(BufferType::Models, index);
}

GPUModel* MStore::getGPUMovingModel(int32_t index) {
	return engine->globalRendering.gpuMemory.getCppBuffer<GPUModel>(BufferType::ModelsMoving, index);
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

const MeshInfoMetadata* MStore::getMeshMetadata(int32_t index) const {
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

void MStore::addToGlobalBuffers(const std::vector<GPUMeshInfo>& gpuMeshInfos, const std::vector<MeshInfoMetadata>& gpuMeshMetadata, const std::vector<GPUMaterial>& gpuMaterialInfos)
{
	// Materials:
    auto globalMaterialStart = engine->globalRendering.gpuMemory.getElementCount(BufferType::Materials);

	for (const auto& material : gpuMaterialInfos) {
		GPUMaterial globalMaterial = material;
		if (globalMaterial.baseColor >= 0)         globalMaterial.baseColor = engine->mstore.gltf.getGlobalTextureIndex(static_cast<int>(material.baseColor));
		if (globalMaterial.emissive >= 0)          globalMaterial.emissive = engine->mstore.gltf.getGlobalTextureIndex(static_cast<int>(material.emissive));
        if (globalMaterial.metallicRoughness >= 0) globalMaterial.metallicRoughness = engine->mstore.gltf.getGlobalTextureIndex(static_cast<int>(material.metallicRoughness));
        if (globalMaterial.normal >= 0)            globalMaterial.normal = engine->mstore.gltf.getGlobalTextureIndex(static_cast<int>(material.normal));
        if (globalMaterial.occlusion >= 0)         globalMaterial.occlusion = engine->mstore.gltf.getGlobalTextureIndex(static_cast<int>(material.occlusion));

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
	if (flags.hasFlag(MeshFlags::RENDER_TYPE_MOVING)) {
		if (mesh_index < 0 || mesh_index >= static_cast<int32_t>(movingSceneObjects.size())) {
			Error("MStore::addObject: Invalid mesh index");
			return nullptr; // keep compiler happy
		}
		// current index:
		auto objectIndex = engine->globalRendering.gpuMemory.getElementCount(BufferType::ModelsMoving);
		if (objectIndex >= movingSceneObjects.size()) Error("MStore::addObject: Exceeded maximum number of moving objects");

		GPUModel* model = getGPUMovingModel(objectIndex);
		SceneObject* obj = getMovingSceneObject(objectIndex);
		obj->index = objectIndex;
		obj->pos = pos;
		model->meshNumber = mesh_index;
		engine->globalRendering.gpuMemory.appendElement(BufferType::ModelsMoving, model);
		return obj;
	} else {
		if (mesh_index < 0 || mesh_index >= static_cast<int32_t>(sceneObjects.size())) {
			Error("MStore::addObject: Invalid mesh index");
			return nullptr; // keep compiler happy
		}
		// current index:
		auto objectIndex = engine->globalRendering.gpuMemory.getElementCount(BufferType::Models);
		if (objectIndex >= sceneObjects.size()) Error("MStore::addObject: Exceeded maximum number of stationary objects");

		GPUModel* model = getGPUModel(objectIndex);
		SceneObject* obj = getSceneObject(objectIndex);
		obj->index = objectIndex;
		obj->pos = pos;
		model->meshNumber = mesh_index;
		engine->globalRendering.gpuMemory.appendElement(BufferType::Models, model);
		return obj;
	}
}
