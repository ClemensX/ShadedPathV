#include "mainheader.h"

using namespace std;
using namespace glm;

void MStore::init() {
	gltf.init(engine);

	// initialize structures on GPU global mesh storage:
	auto* mem = engine->globalRendering.getCurrentGPUMemoryChunk();

	// set array sizes, these are guarded and user needs to adjust them if too small at runtime
	size_t maxMeshes = engine->getMaxMeshes() * 10; // max meshes is a user setting, roughly we need 10 meshes as each 'user mesh' has 10 LOD

	engine->globalRendering.gpuMemory.defineBuffer(BufferType::MeshInfos, sizeof(GPUMeshInfo), maxMeshes);
	engine->globalRendering.gpuMemory.allocateBuffers();
}

void MStore::loadMesh(std::string filename, std::string id, MeshFlagsCollection flags)
{
    assert(engine != nullptr);
	vector<byte> file_buffer;
	loadFile(filename, file_buffer);
    
	// test
	GPUMeshInfo meshInfo{};
    engine->globalRendering.gpuMemory.appendElement(BufferType::MeshInfos, meshInfo);
    size_t count = engine->globalRendering.gpuMemory.getElementCount(BufferType::MeshInfos);
	assert(count > 0);
	const GPUMeshInfo* meshInfoPtr = engine->globalRendering.gpuMemory.getCppBuffer<GPUMeshInfo>(BufferType::MeshInfos, 0);
	assert(meshInfoPtr != nullptr);
	assert(meshInfoPtr->globalIndexOffset == 0);
    assert(meshInfoPtr[0].localIndexOffset == 0);

    gltf.load2((const unsigned char*)file_buffer.data(), (int)file_buffer.size(), filename);
}

void MStore::loadFile(std::string filename, std::vector<std::byte>& fileBuffer)
{
	// find texture file, look in pak file first:
	PakEntry* pakFileEntry = nullptr;
	pakFileEntry = engine->files.findFileInPak(filename.c_str());
	// try file system if not found in pak:
	string binFile;
	if (pakFileEntry == nullptr) {
		binFile = engine->files.findFile(filename.c_str(), FileCategory::MESH);
		engine->files.readFile(binFile, fileBuffer, FileCategory::MESH);
	}
	else {
		engine->files.readFile(pakFileEntry, fileBuffer, FileCategory::MESH);
	}


}
