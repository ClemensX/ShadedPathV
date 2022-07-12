#include "pch.h"

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tinygltf/tiny_gltf.h"

using namespace tinygltf;

void glTF::init(ShadedPathEngine* e) {
	engine = e;
}

// we need to finish texture processing within this method, as tinygltf overwrites some data between calls
bool LoadImageDataKTX(Image* image, const int image_idx, std::string* err,
	std::string* warn, int req_width, int req_height,
	const unsigned char* bytes, int size, void* user_data) {
	auto* userData = (glTF::gltfUserData*) user_data;
	//Log("my image loader" << endl);
	// ab 4b 54 58 20 32 30 bb
	string ktkMagic = "\xabKTX 20\xbb";
	if (size < 10 || strncmp((const char*)bytes, ktkMagic.c_str(), 8) != 0) {
		Error("unexpected texture format, cannot parse");
		return true;
	}
	ktxTexture* kTexture;
	userData->engine->textureStore.createKTXFromMemory(bytes, size, &kTexture);
	// we are not entirely sure that textures will arrive here with ever incresing indices,
	// so we play safe and resize vector of texture pointers, if necessary:
	auto& tvec = userData->mesh->textureParseInfo;
	if (tvec.size() <= image_idx) {
		tvec.resize(image_idx + 1);
		userData->mesh->textureInfos.resize(image_idx + 1);
	}
	tvec[image_idx] = kTexture;
	auto* texture = userData->engine->textureStore.createTextureSlot(userData->mesh, image_idx);
	userData->engine->textureStore.createVulkanTextureFromKTKTexture(kTexture, texture);
	userData->mesh->textureInfos[image_idx] = texture;
	//userData->engine->textureStore.destroyKTXIntermediate(kTexture);
	return true;
}

void glTF::loadModel(Model &model, const unsigned char* data, int size, MeshInfo* mesh, string filename)
{
	TinyGLTF loader;
	string err;
	string warn;
	gltfUserData userData {engine, mesh};
	loader.SetImageLoader(&LoadImageDataKTX, (void*)&userData);
	if (size < 4) {
		Error("invalid glTF file");
	}
	bool isBinary = false;
	if (data[0] == 'g' && data[1] == 'l' && data[2] == 'T' &&
		data[3] == 'F') {
		isBinary = true;
	}

	bool ret;
	if (isBinary) {
		ret = loader.LoadBinaryFromMemory(&model, &err, &warn, data, size);
	}
	else {
		filesystem::path p = filename.c_str();
		string dir = p.parent_path().string();
		ret = loader.LoadASCIIFromString(&model, &err, &warn, (const char*)data, (unsigned int)size, dir);
	}
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
	return;
}

void glTF::loadVertices(tinygltf::Model& model, vector<PBRShader::Vertex>& verts, vector<uint32_t>& indexBuffer)
{
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
			// assert all the content we rely on:
			assert(primitive.attributes.find("POSITION") != primitive.attributes.end());
			assert(primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end());

			// base texture 
			const float* bufferTexCoordSet0 = nullptr;
			const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
			const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
			bufferTexCoordSet0 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
			int uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);


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
				PBRShader::Vertex vert;
				vert.pos = vec3(bufferPos[pos], bufferPos[pos + 1], bufferPos[pos + 2]);
				vert.uv0 = vec2(bufferTexCoordSet0[v * uv0ByteStride], bufferTexCoordSet0[v * uv0ByteStride + 1]);
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

void glTF::loadVertices(const unsigned char* data, int size, vector<PBRShader::Vertex>& verts, vector<uint32_t> &indexBuffer, string filename)
{
	Model model;
	//loadModel(model, data, size, filename);
	loadVertices(model, verts, indexBuffer);
}

void glTF::load(const unsigned char* data, int size, MeshInfo* mesh, string filename)
{
	Model model;
	loadModel(model, data, size, mesh, filename);
	loadVertices(model, mesh->vertices, mesh->indices);
}