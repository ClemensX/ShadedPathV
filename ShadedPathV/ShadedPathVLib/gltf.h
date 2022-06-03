// load gltf models via tinygltf.
// thread sync has to be done outside this class
class glTF {
public:
	static void load(const unsigned char* data, int size);
private:
};