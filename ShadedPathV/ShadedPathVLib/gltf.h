// load gltf models via tinygltf.
// thread sync has to be done outside this class

namespace tinygltf {
	class Model;
}

class glTF {
public:
	static void loadVertices(const unsigned char* data, int size, vector<vec3> &verts, vector<uint32_t> &indexBuffer, string filename);
private:
	static void loadModel(tinygltf::Model& model, const unsigned char* data, int size, string filename);
	// copy model vertices and indices into vectors
	static void loadVertices(tinygltf::Model& model, vector<vec3>& verts, vector<uint32_t>& indexBuffer);
};