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
	size_t maxMaterials = engine->getMaxCollections();

	engine->globalRendering.gpuMemory.defineBuffer(BufferType::MeshInfos, sizeof(GPUMeshInfo), maxMeshes);
	engine->globalRendering.gpuMemory.defineBuffer(BufferType::Models, sizeof(GPUModel), maxModels);
	engine->globalRendering.gpuMemory.defineBuffer(BufferType::Materials, sizeof(GPUMaterial), maxMaterials);
	engine->globalRendering.gpuMemory.allocateBuffers();
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
	const GPUMeshInfo* meshInfoPtr = engine->globalRendering.gpuMemory.getCppBuffer<GPUMeshInfo>(BufferType::MeshInfos, 0);
	for (int i = 0; i < meshNumCount; ++i) {
        MeshFileEntry entry{};
        entry.name = meshInfoPtr[meshNumStart + i].name;
        entry.meshIndex = meshNumStart + i;
        meshFile.meshes.push_back(entry);
    }
    meshFiles.push_back(meshFile);
    addMeshFileID(id, static_cast<int32_t>(meshFiles.size() - 1));
}

const GPUMeshInfo* MStore::getGPUMeshInfo(int32_t index) const {
	return engine->globalRendering.gpuMemory.getCppBuffer<GPUMeshInfo>(BufferType::MeshInfos, index);
}

const GPUMaterial* MStore::getGPUMaterial(int32_t index) const {
	return engine->globalRendering.gpuMemory.getCppBuffer<GPUMaterial>(BufferType::Materials, index);
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

void MStore::addToGlobalBuffers(const std::vector<GPUMeshInfo>& gpuMeshInfos, const std::vector<GPUMaterial>& gpuMaterialInfos)
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
        i++;
    }
}