#include "mainheader.h"

using namespace std;
using namespace glm;

void MStore::loadMesh(std::string filename, std::string id, MeshFlagsCollection flags)
{
    assert(engine != nullptr);
	vector<byte> file_buffer;
	loadFile(filename, file_buffer);

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
