#include "pch.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tinygltf/tiny_gltf.h"

using namespace tinygltf;

void glTF::loadVertices(const unsigned char* data, int size, vector<vec3>& verts)
{
	Model model;
	TinyGLTF loader;
	string err;
	string warn;

	bool ret = loader.LoadBinaryFromMemory(&model, &err, &warn, data, size);
	if (!warn.empty()) {
		printf("Warn: %s\n", warn.c_str());
	}

	if (!err.empty()) {
		printf("Err: %s\n", err.c_str());
	}

	if (!ret) {
		printf("Failed to parse glTF\n");
		return;
	}
	// parse vertices:
	if (model.meshes.size() > 0) {
		const tinygltf::Mesh mesh = model.meshes[0];
		if (mesh.primitives.size() > 0) {
			const tinygltf::Primitive& primitive = mesh.primitives[0];
			if (primitive.indices > -1) {
				const float* bufferPos = nullptr;
				uint32_t vertexCount = 0;
				glm::vec3 posMin{};
				glm::vec3 posMax{};
				int posByteStride;

				primitive.attributes.find("POSITION");
				const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
				const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
				bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
				posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
				posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
				vertexCount = static_cast<uint32_t>(posAccessor.count);
				auto str = posAccessor.ByteStride(posView);
				//Log("stride " << str << endl);
				posByteStride = posAccessor.ByteStride(posView) / sizeof(float);
				Log("posByteStride " << posByteStride << endl);
				for (size_t v = 0; v < posAccessor.count-1; v++) {
					size_t pos = v * posByteStride;
					vec3 vert = vec3(bufferPos[pos], bufferPos[pos + 1], bufferPos[pos + 2]);
					vec3 vert2 = vec3(bufferPos[pos + 3], bufferPos[pos + 4], bufferPos[pos + 5]);
					verts.push_back(vert);
					verts.push_back(vert2);
					//Log("vert " << vert.x << endl);
				}
			}
		}
	}
}