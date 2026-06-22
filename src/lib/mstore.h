// new mesh store

#pragma once

class MStore : public EngineParticipant {
public:
    void setLimits(size_t maxMeshes) {
        this->maxMeshes = maxMeshes;
    }
	// load all meshes from glTF file, objects are referenced via id string according to this schema:
	// ref string == gltf mesh
	// =======================
	// id == mesh[0]
	// id.gltf_mesh_name == mesh with name == gltf_mesh_name
	// id.2 == mesh[2]
	void loadMesh(std::string filename, std::string id, MeshFlagsCollection flags = MeshFlagsCollection());
private:
    size_t maxMeshes = 0; // maximum number of meshes that can be stored, set setLimits()
    void loadFile(std::string filename, std::vector<std::byte>& fileBuffer);
};