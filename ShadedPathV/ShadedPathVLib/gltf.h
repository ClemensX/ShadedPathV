// load gltf models via tinygltf.
// thread sync has to be done outside this class

// forward declarations:
namespace tinygltf {
	class Model;
}
struct MeshInfo;

class glTF {
public:
	void init(ShadedPathEngine* e);
	void loadVertices(const unsigned char* data, int size, vector<vec3>& verts, vector<uint32_t>& indexBuffer, string filename);
	void load(const unsigned char* data, int size, MeshInfo *mesh, string filename);
	// used for hook into tinygltf image loading:
	struct gltfUserData {
		ShadedPathEngine* engine = nullptr;
		MeshInfo* mesh = nullptr;
	};
private:
	void loadModel(tinygltf::Model& model, const unsigned char* data, int size, MeshInfo* mesh, string filename);
	// copy model vertices and indices into vectors
	// index buffer will be 32 bit wide in all cases (VK_INDEX_TYPE_UINT32)
	void loadVertices(tinygltf::Model& model, vector<vec3>& verts, vector<uint32_t>& indexBuffer);
	ShadedPathEngine* engine = nullptr;
};