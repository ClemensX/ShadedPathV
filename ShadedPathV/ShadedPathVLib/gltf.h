// load gltf models via tinygltf.
// thread sync has to be done outside this class
class glTF {
public:
	static void loadVertices(const unsigned char* data, int size, vector<vec3> &verts);
private:
};