#include "pch.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tinygltf/tiny_gltf.h"

using namespace tinygltf;

void glTF::loadVertices(const unsigned char* data, int size, vector<vec3>& verts, vector<uint32_t> &indexBuffer)
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

	// should be sized of passed in vectors:
	uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
	uint32_t vertexStart = static_cast<uint32_t>(verts.size());

	// parse vertices, indexes:
	if (model.meshes.size() > 0) {
		const tinygltf::Mesh mesh = model.meshes[0];
		if (mesh.primitives.size() > 0) {
			const tinygltf::Primitive& primitive = mesh.primitives[0];
			bool hasIndices = primitive.indices > -1;
			if (!hasIndices) {
				Error("Cannot parse mesh without indices");
			}
			// parse vertices
			const float* bufferPos = nullptr;
			uint32_t vertexCount = 0;
			glm::vec3 posMin{};
			glm::vec3 posMax{};
			int posByteStride;

			auto position = primitive.attributes.find("POSITION");
			const tinygltf::Accessor& posAccessor = model.accessors[position->second];
			const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
			bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
			posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
			posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
			vertexCount = static_cast<uint32_t>(posAccessor.count);
			auto str = posAccessor.ByteStride(posView);
			//Log("stride " << str << endl);
			posByteStride = posAccessor.ByteStride(posView) / sizeof(float);
			Log("posByteStride " << posByteStride << endl);
			for (size_t v = 0; v < posAccessor.count; v++) {
				size_t pos = v * posByteStride;
				vec3 vert = vec3(bufferPos[pos], bufferPos[pos + 1], bufferPos[pos + 2]);
				//vec3 vert2 = vec3(bufferPos[pos + 3], bufferPos[pos + 4], bufferPos[pos + 5]);
				verts.push_back(vert);
				//verts.push_back(vert2);
				//Log("vert " << vert.x << endl);
			}

			// parse indices:
			const tinygltf::Accessor& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

			uint32_t indexCount = static_cast<uint32_t>(accessor.count);
			const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

			switch (accessor.componentType) {
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
				const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
				for (size_t index = 0; index < accessor.count; index++) {
					indexBuffer.push_back(buf[index] + vertexStart);
				}
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
				const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
				for (size_t index = 0; index < accessor.count; index++) {
					indexBuffer.push_back(buf[index] + vertexStart);
				}
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
				const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
				for (size_t index = 0; index < accessor.count; index++) {
					indexBuffer.push_back(buf[index] + vertexStart);
				}
				break;
			}
			default:
				Error("Index component type not supported!");
			}
			Log("Verts loaded: " << verts.size() << endl);
			Log("Indices loaded: " << indexBuffer.size() << endl);
			assert(indexBuffer.size() % 3 == 0); // triangles?
		}
		//if (primitive.indices > -1) {
		//}
		//}
	}
}