#include "mainheader.h"

using namespace std;

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#if defined(_WIN64)
#define STBI_MSC_SECURE_CRT
#endif
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
// change warning level - we do not want to see minaudio warnings
#pragma warning( push, 2 )

#pragma warning(disable:33005)
#pragma warning(disable:6262)
#pragma warning(disable:6011)
#pragma warning(disable:6385)
#pragma warning(disable:26451)
#pragma warning(disable:6255)
#pragma warning(disable:6386)
#pragma warning(disable:6001)

#include "tinygltf/tiny_gltf.h"

#pragma warning( pop )

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
	auto& tvec = userData->collection->textureParseInfo;
	if (tvec.size() <= image_idx) {
		tvec.resize(image_idx + 1);
		userData->collection->textureInfos.resize(image_idx + 1);
	}
	tvec[image_idx] = kTexture;
	auto* texture = userData->engine->textureStore.createTextureSlotForMesh(userData->collection->meshInfos[0], image_idx);
    //texture->type = TextureType::TEXTURE_TYPE_GLTF;
	userData->engine->textureStore.createVulkanTextureFromKTKTexture(kTexture, texture);
	userData->collection->textureInfos[image_idx] = texture;
	//userData->engine->textureStore.destroyKTXIntermediate(kTexture);
	ktxTexture_Destroy(kTexture);
	return true;
}

void glTF::loadModel(Model &model, const unsigned char* data, int size, MeshCollection* coll, string filename)
{
	TinyGLTF loader;
	string err;
	string warn;
	gltfUserData userData {engine, coll};
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
		Error("Failed to parse glTF");
		return;
	}
	return;
}

// Add after includes and before existing functions (e.g., near other static helpers).

struct KHRTextureTransform {
	bool present = false;
	glm::vec2 offset{ 0.0f, 0.0f };
	glm::vec2 scale{ 1.0f, 1.0f };
	float rotation = 0.0f; // radians
	int texCoordOverride = -1; // -1 means use TextureInfo.texCoord
};

static KHRTextureTransform ParseKHRTextureTransform(const tinygltf::ExtensionMap& extMap) {
	KHRTextureTransform t{};
	auto it = extMap.find("KHR_texture_transform");
	if (it == extMap.end()) return t;

	const tinygltf::Value& ext = it->second;
	t.present = true;

	if (ext.Has("offset")) {
		const tinygltf::Value& arr = ext.Get("offset");
		if (arr.IsArray() && arr.ArrayLen() >= 2) {
			t.offset.x = static_cast<float>(arr.Get(0).Get<double>());
			t.offset.y = static_cast<float>(arr.Get(1).Get<double>());
		}
	}
	if (ext.Has("scale")) {
		const tinygltf::Value& arr = ext.Get("scale");
		if (arr.IsArray() && arr.ArrayLen() >= 2) {
			t.scale.x = static_cast<float>(arr.Get(0).Get<double>());
			t.scale.y = static_cast<float>(arr.Get(1).Get<double>());
		}
	}
	if (ext.Has("rotation")) {
		t.rotation = static_cast<float>(ext.Get("rotation").Get<double>());
	}
	if (ext.Has("texCoord")) {
		t.texCoordOverride = ext.Get("texCoord").Get<int>();
	}
	return t;
}

static inline int ResolveTexCoordUsed(const int tiTexCoord, const KHRTextureTransform& t) {
	return (t.texCoordOverride >= 0) ? t.texCoordOverride : tiTexCoord;
}

static inline glm::vec2 ApplyTransformToUV(glm::vec2 uv, const KHRTextureTransform& t) {
	// Spec order: scale -> rotate -> translate, pivot at origin.
	uv *= t.scale;
	if (t.rotation != 0.0f) {
		float c = std::cos(t.rotation);
		float s = std::sin(t.rotation);
		uv = glm::vec2(c * uv.x - s * uv.y, s * uv.x + c * uv.y);
	}
	uv += t.offset;
	return uv;
}

static inline bool NearlyEqual(float a, float b, float eps = 1e-6f) {
	return std::abs(a - b) <= eps;
}
static bool SameTransform(const KHRTextureTransform& a, const KHRTextureTransform& b) {
	return a.present == b.present &&
		NearlyEqual(a.offset.x, b.offset.x) &&
		NearlyEqual(a.offset.y, b.offset.y) &&
		NearlyEqual(a.scale.x, b.scale.x) &&
		NearlyEqual(a.scale.y, b.scale.y) &&
		NearlyEqual(a.rotation, b.rotation) &&
		a.texCoordOverride == b.texCoordOverride;
}

static void ApplyTransformToMeshUVChannel(std::vector<PBRShader::Vertex>& verts, int uvSet, const KHRTextureTransform& t) {
	if (!t.present) return;
	if (uvSet < 0) return;
	for (auto& v : verts) {
		if (uvSet == 0) {
			v.uv0 = ApplyTransformToUV(v.uv0, t);
		}
		else if (uvSet == 1) {
			v.uv1 = ApplyTransformToUV(v.uv1, t);
		}
		else {
			// Only TEXCOORD_0 and TEXCOORD_1 are supported by current vertex format.
			// Silently ignore higher channels.
		}
	}
}

static inline float DecodeComponent(int componentType, bool normalized, const void* src)
{
	switch (componentType) {
	case TINYGLTF_COMPONENT_TYPE_FLOAT: {
		float v;
		std::memcpy(&v, src, sizeof(float));
		return v;
	}
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
		uint8_t v;
		std::memcpy(&v, src, 1);
		return normalized ? (float)v / 255.0f : (float)v;
	}
	case TINYGLTF_COMPONENT_TYPE_BYTE: {
		int8_t v;
		std::memcpy(&v, src, 1);
		if (normalized) {
			// Map to [-1,1]
			return std::max(-1.0f, (float)v / 127.0f);
		}
		return (float)v;
	}
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
		uint16_t v;
		std::memcpy(&v, src, 2);
		return normalized ? (float)v / 65535.0f : (float)v;
	}
	case TINYGLTF_COMPONENT_TYPE_SHORT: {
		int16_t v;
		std::memcpy(&v, src, 2);
		if (normalized) {
			return std::max(-1.0f, (float)v / 32767.0f);
		}
		return (float)v;
	}
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
		uint32_t v;
		std::memcpy(&v, src, 4);
		return (float)v; // POSITION should not be UINT normally, but handle generically.
	}
	default:
		Log("Unsupported vertex componentType: " << componentType << "\n");
		return 0.0f;
	}
}

void extractVertexAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const std::string& attributeName, std::vector<float>& outData, int& stride) {
	outData.clear();
	stride = 0;

	auto it = primitive.attributes.find(attributeName);
	if (it == primitive.attributes.end()) return;

	const tinygltf::Accessor& accessor = model.accessors[it->second];
	if (accessor.bufferView < 0) {
		Log("Accessor bufferView < 0 for attribute " << attributeName << "\n");
		return;
	}

	const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
	const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

	const size_t numComponents = tinygltf::GetNumComponentsInType(accessor.type);
	if (numComponents == 0) {
		Log("Invalid accessor.type for attribute " << attributeName << "\n");
		return;
	}

	// Bytes per single component.
	int componentSize = 0;
	switch (accessor.componentType) {
	case TINYGLTF_COMPONENT_TYPE_FLOAT:          componentSize = 4; break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
	case TINYGLTF_COMPONENT_TYPE_BYTE:           componentSize = 1; break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
	case TINYGLTF_COMPONENT_TYPE_SHORT:          componentSize = 2; break;
	case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   componentSize = 4; break;
	default:
		Log("Unsupported vertex componentType: " << accessor.componentType << "\n");
		return;
	}

	// Byte stride between consecutive vertices in the buffer.
	const int byteStride = accessor.ByteStride(bufferView) > 0
		? accessor.ByteStride(bufferView)
		: int(numComponents) * componentSize;

	if (byteStride < componentSize * int(numComponents)) {
		// Spec allows >= packed size. Anything smaller is invalid.
		Log("Invalid byteStride (" << byteStride << ") for attribute " << attributeName << "\n");
		return;
	}

	const size_t start = bufferView.byteOffset + accessor.byteOffset;
	// Last vertex must fit: start + (count-1)*byteStride + packedSize
	const size_t packedSize = numComponents * size_t(componentSize);
	const size_t lastByte = start + (accessor.count ? (accessor.count - 1) * size_t(byteStride) : 0) + packedSize;
	if (lastByte > buffer.data.size()) {
		Log("Buffer overrun risk while reading attribute " << attributeName << "\n");
		return;
	}

	const unsigned char* basePtr = reinterpret_cast<const unsigned char*>(buffer.data.data() + start);

	outData.resize(accessor.count * numComponents);
	stride = int(numComponents); // number of float components per vertex

	for (size_t i = 0; i < accessor.count; ++i) {
		const unsigned char* elem = basePtr + i * byteStride;
		// Components in the attribute are ALWAYS tightly packed starting at elem,
		// even if vertex is interleaved (extra bytes follow after the attribute data).
		for (size_t c = 0; c < numComponents; ++c) {
			const void* compSrc = elem + c * componentSize;
			outData[i * numComponents + c] = DecodeComponent(accessor.componentType, accessor.normalized, compSrc);
		}
	}
}

void glTF::loadVertices(tinygltf::Model& model, MeshInfo* mesh, std::vector<PBRShader::Vertex>& verts, std::vector<uint32_t>& indexBuffer, int gltfMeshIndex) {
	assert(mesh->gltfMeshIndex >= 0);
	assert(mesh->gltfMeshIndex == gltfMeshIndex);

	uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
	uint32_t vertexStart = static_cast<uint32_t>(verts.size());

	if (model.meshes.size() > 0) {
		const tinygltf::Mesh& gltfMesh = model.meshes[gltfMeshIndex];
		if (gltfMesh.primitives.size() > 0) {
			const tinygltf::Primitive& primitive = gltfMesh.primitives[0];
			bool hasIndices = primitive.indices > -1;
			if (!hasIndices) {
				Error("Cannot parse mesh without indices");
			}

			// Extract positions
			std::vector<float> positions;
			int posStride;
			extractVertexAttribute(model, primitive, "POSITION", positions, posStride);

			// Extract colors
			std::vector<float> colors;
			int colorStride;
			extractVertexAttribute(model, primitive, "COLOR_0", colors, colorStride);

			// Extract texture coordinates
			std::vector<float> texCoords;
			int texCoordStride;
			extractVertexAttribute(model, primitive, "TEXCOORD_0", texCoords, texCoordStride);

			// Extract 2nd texture coordinates
			std::vector<float> texCoords2;
			int texCoordStride2;
			extractVertexAttribute(model, primitive, "TEXCOORD_1", texCoords2, texCoordStride2);

			// Extract normals
			std::vector<float> normals;
			int normalStride;
			extractVertexAttribute(model, primitive, "NORMAL", normals, normalStride);

            // Populate vertices, all floats are auto initialized to 0.0f
			for (size_t v = 0; v < positions.size() / posStride; v++) {
				PBRShader::Vertex vert{};
				vert.pos = glm::vec3(positions[v * posStride], positions[v * posStride + 1], positions[v * posStride + 2]);

				if (!colors.empty()) {
					vert.color = glm::vec4(colors[v * colorStride], colors[v * colorStride + 1], colors[v * colorStride + 2], colors[v * colorStride + 3]);
				}
				else {
					vert.color = glm::vec4(1.0f); // Default to white if no color attribute
				}

				if (!texCoords.empty()) {
					vert.uv0 = glm::vec2(texCoords[v * texCoordStride], texCoords[v * texCoordStride + 1]);
				}
				if (!texCoords2.empty()) {
					vert.uv1 = glm::vec2(texCoords2[v * texCoordStride2], texCoords2[v * texCoordStride2 + 1]);
				}
				if (!normals.empty()) {
                    vert.normal = glm::vec3(normals[v * normalStride], normals[v * normalStride + 1], normals[v * normalStride + 2]);
                }

				verts.push_back(vert);
			}

			// Parse indices
			const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
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
			assert(indexBuffer.size() % 3 == 0); // Ensure triangles

			if (mesh->flags.hasFlag(MeshFlags::MESH_TYPE_FLIP_WINDING_ORDER)) {
				// Flip winding order
				for (size_t i = 0; i < indexBuffer.size(); i += 3) {
					std::swap(indexBuffer[i], indexBuffer[i + 2]);
				}
			}
		}
	}
}

//void setTextureData(tinygltf::Model& model, MeshInfo* mesh, string type) {
//	::TextureInfo** texAdr = nullptr;
//	if (glTF::BASE_COLOR_TEXTURE.compare(type) == 0) {
//		texAdr = &mesh->baseColorTexture;
//	} else {
//		//Error("unexpected texture type encountered");
//		Log("unexpected texture type encountered" << endl);
//	}
//	auto& mat = model.materials[0];
//	if (mat.values.find(type) != mat.values.end()) {
//		//*texAdr = mesh->textureInfos[mat.values["baseColorTexture"].TextureIndex()];
//		int coordsIndex = mat.values["baseColorTexture"].TextureTexCoord();
//		Log("coord index " << coordsIndex << endl);
//	}
//}
//

// determine data pointer and stride for gltf texture and store in our TextureInfo
void getUVCoordinates(tinygltf::Model& model, tinygltf::Primitive& primitive, ::TextureInfo* texInfo, string selector) {
	const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find(selector)->second];
	const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
	texInfo->gltfTexCoordData = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
	texInfo->gltfUVByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
}

// determine data pointer and stride for gltf texture and store in our TextureInfo
void getTextureUVCoordinates(tinygltf::Model& model, tinygltf::Primitive& primitive, ::TextureInfo* texInfo, int texCoord) {
	assert(texCoord >= 0);
	string texCoordSelector = string("TEXCOORD_") + to_string(texCoord);
	getUVCoordinates(model, primitive, texInfo, texCoordSelector);
}

void glTF::prepareTexturesAndMaterials(tinygltf::Model& model, MeshCollection* coll, int gltfMeshIndex)
{
	// make sure textures are already loded
	assert(coll->textureInfos.size() >= model.samplers.size());

    // we have to parse gltf textures and look for KHR_texture_basisu extension,
    // then set the source index of the texture accordingly
	for (int i = 0; i < model.textures.size(); i++) {
		auto& extMap = model.textures[i].extensions;
		auto found = extMap.find("KHR_texture_basisu");
		if (found != extMap.end()) {
			if (found->second.Has("source")) {
				int source = found->second.Get("source").Get<int>();
				//Log("source " << source << endl);
				model.textures[i].source = source;
			}
		}
	}
    // make sure we have a valid source index for all textures
	for (int i = 0; i < model.textures.size(); i++) {
		auto& extMap = model.textures[i].extensions;
        assert(model.textures[i].source >= 0);
	}
	auto& primitive = model.meshes[gltfMeshIndex].primitives[0];
	auto& mat = model.materials[primitive.material];
	// assign mesh textures to pre-loaded texture index:
	auto baseColorTextureIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
	auto metallicRoughnessTextureIndex = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
	auto normalTextureIndex = mat.normalTexture.index;
	auto occlusionTextureIndex = mat.occlusionTexture.index;
	auto emissiveTextureIndex = mat.emissiveTexture.index;

	MeshInfo* mesh = coll->meshInfos[gltfMeshIndex];
    mesh->metallicRoughness = true; // default to metallic roughness workflow
    mesh->isDoubleSided = mat.doubleSided;

	// create samplers
	vector<VkSampler> samplers(model.samplers.size());
	for (int i = 0; i < model.samplers.size(); i++) {
        VkSamplerCreateInfo vkSamplerInfo{};
        mapTinyGLTFSamplerToVulkan(model.samplers[i], vkSamplerInfo);
        samplers[i] = engine->globalRendering.samplerCache.getOrCreateSampler(engine->globalRendering.device, vkSamplerInfo);
        //Log("sampler: " << model.samplers[i].name.c_str() << endl);
	}

	// Parse KHR_texture_transform and resolve final texCoord per texture use
	KHRTextureTransform tfBase, tfMR, tfNormal, tfOcc, tfEmi;
	int tcBase = -1, tcMR = -1, tcNormal = -1, tcOcc = -1, tcEmi = -1;

	if (baseColorTextureIndex >= 0) {
		tfBase = ParseKHRTextureTransform(mat.pbrMetallicRoughness.baseColorTexture.extensions);
		tcBase = ResolveTexCoordUsed(mat.pbrMetallicRoughness.baseColorTexture.texCoord, tfBase);
	}
	if (metallicRoughnessTextureIndex >= 0) {
		tfMR = ParseKHRTextureTransform(mat.pbrMetallicRoughness.metallicRoughnessTexture.extensions);
		tcMR = ResolveTexCoordUsed(mat.pbrMetallicRoughness.metallicRoughnessTexture.texCoord, tfMR);
	}
	if (normalTextureIndex >= 0) {
		tfNormal = ParseKHRTextureTransform(mat.normalTexture.extensions);
		tcNormal = ResolveTexCoordUsed(mat.normalTexture.texCoord, tfNormal);
	}
	if (occlusionTextureIndex >= 0) {
		tfOcc = ParseKHRTextureTransform(mat.occlusionTexture.extensions);
		tcOcc = ResolveTexCoordUsed(mat.occlusionTexture.texCoord, tfOcc);
	}
	if (emissiveTextureIndex >= 0) {
		tfEmi = ParseKHRTextureTransform(mat.emissiveTexture.extensions);
		tcEmi = ResolveTexCoordUsed(mat.emissiveTexture.texCoord, tfEmi);
	}

	// Bake transforms into vertex UVs per channel (only TEXCOORD_0 and TEXCOORD_1 supported).
	// If multiple textures require different transforms on the same UV set, the first one wins; a warning is logged.
	std::optional<KHRTextureTransform> perSet[2];

	auto consider = [&](int tc, const KHRTextureTransform& t, const char* usage) {
		if (tc < 0 || tc > 1 || !t.present) return;
		if (!perSet[tc].has_value()) {
			perSet[tc] = t;
		}
		else if (!SameTransform(perSet[tc].value(), t)) {
			Log(std::string("WARNING: Different KHR_texture_transform for UV set ") + std::to_string(tc) +
				" between textures; keeping first and ignoring '" + usage + "' transform\n");
		}
		};

	consider(tcBase, tfBase, "baseColor");
	consider(tcMR, tfMR, "metallicRoughness");
	consider(tcNormal, tfNormal, "normal");
	consider(tcOcc, tfOcc, "occlusion");
	consider(tcEmi, tfEmi, "emissive");

	if (perSet[0].has_value()) {
		ApplyTransformToMeshUVChannel(mesh->vertices, 0, perSet[0].value());
	}
	if (perSet[1].has_value()) {
		ApplyTransformToMeshUVChannel(mesh->vertices, 1, perSet[1].value());
	}

	// Now bind textures and pass the correct (possibly overridden) texCoord to TextureInfo
	if (baseColorTextureIndex >= 0) {
		mesh->baseColorTexture = coll->textureInfos[model.textures[baseColorTextureIndex].source];
		getTextureUVCoordinates(model, primitive, mesh->baseColorTexture, std::max(0, tcBase));
		mesh->baseColorTexture->sampler = samplers[model.textures[baseColorTextureIndex].sampler];
		mesh->baseColorTexture->type = TextureType::TEXTURE_TYPE_GLTF;
	}
	if (metallicRoughnessTextureIndex >= 0) {
		mesh->metallicRoughnessTexture = coll->textureInfos[model.textures[metallicRoughnessTextureIndex].source];
		getTextureUVCoordinates(model, primitive, mesh->metallicRoughnessTexture, std::max(0, tcMR));
		mesh->metallicRoughnessTexture->sampler = samplers[model.textures[metallicRoughnessTextureIndex].sampler];
		mesh->metallicRoughnessTexture->type = TextureType::TEXTURE_TYPE_GLTF;
	}
	if (normalTextureIndex >= 0) {
		mesh->normalTexture = coll->textureInfos[model.textures[normalTextureIndex].source];
		getTextureUVCoordinates(model, primitive, mesh->normalTexture, std::max(0, tcNormal));
		mesh->normalTexture->sampler = samplers[model.textures[normalTextureIndex].sampler];
		mesh->normalTexture->type = TextureType::TEXTURE_TYPE_GLTF;
	}
	if (occlusionTextureIndex >= 0) {
		mesh->occlusionTexture = coll->textureInfos[model.textures[occlusionTextureIndex].source];
		getTextureUVCoordinates(model, primitive, mesh->occlusionTexture, std::max(0, tcOcc));
		mesh->occlusionTexture->sampler = samplers[model.textures[occlusionTextureIndex].sampler];
		mesh->occlusionTexture->type = TextureType::TEXTURE_TYPE_GLTF;
	}
	if (emissiveTextureIndex >= 0) {
		mesh->emissiveTexture = coll->textureInfos[model.textures[emissiveTextureIndex].source];
		// BUGFIX: was using occlusionTexture; must pass emissiveTexture
		getTextureUVCoordinates(model, primitive, mesh->emissiveTexture, std::max(0, tcEmi));
		mesh->emissiveTexture->sampler = samplers[model.textures[emissiveTextureIndex].sampler];
		mesh->emissiveTexture->type = TextureType::TEXTURE_TYPE_GLTF;
	}

	if (mesh->flags.hasFlag(MeshFlags::MESH_TYPE_NO_TEXTURES)) {
		return;
	}
	for (auto* tp : coll->textureInfos) {
		engine->textureStore.setTextureActive(tp->id, true);
	}

	// now set the shaderMaterial fields from gltf material:
	PBRShader::ShaderMaterial m{};
	// Use final, possibly overridden texCoord indices
	m.texCoordSets.baseColor = std::max(0, tcBase);
	m.texCoordSets.metallicRoughness = std::max(0, tcMR);
	m.texCoordSets.normal = std::max(0, tcNormal);
	m.texCoordSets.occlusion = std::max(0, tcOcc);
	m.texCoordSets.emissive = std::max(0, tcEmi);

	//m.texCoordSets.baseColor = 1;
	//m.texCoordSets.metallicRoughness = 2;
	//m.texCoordSets.specularGlossiness = 3;
	m.emissiveFactor = glm::vec4(mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2], 1.0f);
    // these are the local texture indexes (within the mesh). Will be overwritten with global texture indexes in prefillModelParameters()
	m.baseColorTextureSet = baseColorTextureIndex;
	m.normalTextureSet = normalTextureIndex;
	m.occlusionTextureSet = occlusionTextureIndex;
	m.emissiveTextureSet = emissiveTextureIndex;
	// values from possible extensions:
	if (mat.extensions.find("KHR_materials_unlit") != mat.extensions.end()) {
		//m.unlit = true;
	}
	m.emissiveStrength = 1.0f; // default
	if (mat.extensions.find("KHR_materials_emissive_strength") != mat.extensions.end()) {
		auto ext = mat.extensions.find("KHR_materials_emissive_strength");
		if (ext->second.Has("emissiveStrength")) {
			auto value = ext->second.Get("emissiveStrength");
			m.emissiveStrength = (float)value.Get<double>();
		}
	}
	if (mesh->isMetallicRoughness()) {
        m.workflow = 0.0f; // metallic roughness workflow
        m.baseColorFactor = glm::vec4(mat.pbrMetallicRoughness.baseColorFactor[0], mat.pbrMetallicRoughness.baseColorFactor[1], mat.pbrMetallicRoughness.baseColorFactor[2], mat.pbrMetallicRoughness.baseColorFactor[3]);
        m.metallicFactor = mat.pbrMetallicRoughness.metallicFactor;
        m.roughnessFactor = mat.pbrMetallicRoughness.roughnessFactor;
        m.physicalDescriptorTextureSet = metallicRoughnessTextureIndex;
        m.baseColorTextureSet = baseColorTextureIndex;
	}
	else Error("only metallic roughness workflow supported");

    // handle alpha mode
	if (mat.alphaMode == "MASK") {
		m.alphaMask = 1.0f;
		m.alphaMaskCutoff = mat.alphaCutoff;
	} else if (mat.alphaMode == "BLEND") {
		// Approximate BLEND mode by setting a low alphaCutoff value
		m.alphaMask = 1.0f;
		m.alphaMaskCutoff = 0.1; // Set a low cutoff value for blending approximation
        Log("WARNING: BLEND alpha mode not supported in " << mesh->id << ", approximated as MASK with alpha cutoff of 0.1" << endl);
	} else {
        assert(mat.alphaMode == "OPAQUE");
        m.alphaMask = 0.0f; // disables alpha masking in shader
    }


	mesh->material = m;
}

void glTF::validateModel(tinygltf::Model& model, MeshCollection* coll)
{
	//assert(model.materials.size() == 1);
	//assert(model.meshes.size() == 1);
	// link back from our mesh to gltf mesh:
	//size_t index = 0;
	// verify Metallic Roughness format 
	for (auto& mat : model.materials) {
		//Log("pbr base Color index: " << pbrbaseColorIndex << endl);
		// check pbr textures availability
		if (!coll->flags.hasFlag(MeshFlags::MESH_TYPE_NO_TEXTURES)) {
			auto texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
			if (texIndex < 0) {
				stringstream s;
				s << "gltf baseColorTexture not found: " << coll->filename << ". try gltf-transform metalrough infile outfile, or mark mesh as MESH_TYPE_NO_TEXTURES to load without textures" << endl;
				Log(s.str());
			}
			texIndex = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
			if (texIndex < 0) {
				stringstream s;
				s << "gltf metallicRoughnessTexture not found: " << coll->filename << ". try gltf-transform metalrough infile outfile, or mark mesh as MESH_TYPE_NO_TEXTURES to load without textures" << endl;
				Log(s.str());
			}
			texIndex = mat.normalTexture.index;
			if (texIndex < 0) {
				stringstream s;
				s << "gltf normalTexture not found: " << coll->filename << ". try gltf-transform metalrough infile outfile, or mark mesh as MESH_TYPE_NO_TEXTURES to load without textures" << endl;
				Log(s.str());
			}
			texIndex = mat.occlusionTexture.index;
			if (texIndex < 0) {
				stringstream s;
				s << "gltf occlusionTexture not found: " << coll->filename << ". try gltf-transform metalrough infile outfile, or mark mesh as MESH_TYPE_NO_TEXTURES to load without textures" << endl;
				Log(s.str());
			}
			texIndex = mat.emissiveTexture.index;
			if (texIndex < 0) {
				stringstream s;
				s << "gltf emissiveTexture not found: " << coll->filename << ". try gltf-transform metalrough infile outfile, or mark mesh as MESH_TYPE_NO_TEXTURES to load without textures" << endl;
				Log(s.str());
			}
		}
		if (mat.doubleSided == true) {
			Log("INFO: double sided gltf material used " << mat.name.c_str() << endl)
		}
	}
	for (auto& m : model.meshes) {
		//Log("  " << m.name.c_str() << endl);
		size_t size = m.primitives.size();
		//Log("  primitives: " << size << endl);
		if (size != 1) {
			stringstream s;
			s << "gltf meshes need to have exactly one primitive: " << coll->filename << " " << m.name.c_str() << endl;
			Error(s.str());
		}
	}
}

// public methods

void glTF::loadVertices(const unsigned char* data, int size, MeshInfo* mesh, vector<PBRShader::Vertex>& verts, vector<uint32_t> &indexBuffer, string filename)
{
	Model model;
	loadVertices(model, mesh, verts, indexBuffer, 0);
}


static glm::mat4 BuildLocalNodeMatrix(const tinygltf::Node& node) {
	// glTF spec:
	// If node.matrix is provided (16 floats), use that directly.
	// Otherwise compose TRS where rotation is a quaternion [x, y, z, w].
	if (node.matrix.size() == 16) {
		glm::mat4 m(1.0f);
		// glTF uses column-major like GLM; values listed in array in column-major order.
		// tinygltf keeps them in same order.
		for (int col = 0; col < 4; ++col) {
			for (int row = 0; row < 4; ++row) {
				m[col][row] = static_cast<float>(node.matrix[col * 4 + row]);
			}
		}
		return m;
	}

	glm::mat4 m(1.0f);

	if (node.translation.size() == 3) {
		m = glm::translate(m, glm::vec3(
			static_cast<float>(node.translation[0]),
			static_cast<float>(node.translation[1]),
			static_cast<float>(node.translation[2])));
	}

	if (node.rotation.size() == 4) {
		// glTF rotation is [x, y, z, w]; GLM quat constructor is (w, x, y, z).
		glm::quat q(
			static_cast<float>(node.rotation[3]),
			static_cast<float>(node.rotation[0]),
			static_cast<float>(node.rotation[1]),
			static_cast<float>(node.rotation[2]));
		m *= glm::toMat4(q);
	}

	if (node.scale.size() == 3) {
		m = glm::scale(m, glm::vec3(
			static_cast<float>(node.scale[0]),
			static_cast<float>(node.scale[1]),
			static_cast<float>(node.scale[2])));
	}

	return m;
}

static glm::mat4 ComputeWorldMatrixForMesh(const tinygltf::Model& model, int meshIndex) {
	// Build child->parent map.
	std::unordered_map<int, int> parentOf;
	parentOf.reserve(model.nodes.size());
	for (int i = 0; i < static_cast<int>(model.nodes.size()); ++i) {
		const tinygltf::Node& n = model.nodes[i];
		for (int c : n.children) {
			parentOf[c] = i;
		}
	}

	int foundNode = -1;
	for (int i = 0; i < static_cast<int>(model.nodes.size()); ++i) {
		const tinygltf::Node& n = model.nodes[i];
		if (n.mesh == meshIndex) {
			if (foundNode != -1) {
				Error("gltf model has multiple nodes referencing the same mesh; not supported for baking.");
			}
			foundNode = i;
		}
	}
	if (foundNode == -1) {
		// No node references this mesh: return identity
		return glm::mat4(1.0f);
	}

	// Collect chain bottom-up.
	std::vector<int> chain;
	int cur = foundNode;
	while (cur >= 0) {
		chain.push_back(cur);
		auto it = parentOf.find(cur);
		if (it == parentOf.end()) break;
		cur = it->second;
	}

	// Build world: parent first.
	glm::mat4 world(1.0f);
	for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
		world = world * BuildLocalNodeMatrix(model.nodes[*it]);
	}
	return world;
}

static void BakeWorldTransformIntoVertices(const glm::mat4& world, std::vector<PBRShader::Vertex>& verts) {
	if (verts.empty()) return;

	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(world)));

	for (auto& v : verts) {
		glm::vec4 p = world * glm::vec4(v.pos, 1.0f);
		v.pos = glm::vec3(p);
		// Only transform normal if it is non-zero
		if (glm::length2(v.normal) > 0.0f) {
			v.normal = glm::normalize(normalMatrix * v.normal);
		}
	}
}

void glTF::collectBaseTransform(tinygltf::Model& model, MeshInfo* mesh)
{
	// Compute world matrix for the node referencing this mesh.
	glm::mat4 world = ComputeWorldMatrixForMesh(model, mesh->gltfMeshIndex);

	// Store for reference (if other systems still expect baseTransform).
	mesh->baseTransform = world;

	// Bake into vertex data.
	if (!mesh->vertices.empty()) {
		//BakeWorldTransformIntoVertices(world, mesh->vertices);
	}

	// After baking, downstream code should treat mesh->baseTransform as already applied.
	// If desired, uncomment the next line to neutralize further usage:
	// mesh->baseTransform = glm::mat4(1.0f);
}

void glTF::load(const unsigned char* data, int size, MeshCollection* coll, string filename)
{
	Model model;
	// parse full gltf file with all meshes and textures. Textures are already pre-loaded into texture store
	loadModel(model, data, size, coll, filename);
	validateModel(model, coll);
	// at this point all textures of gltf file are loaded. info: mesh->textureInfos[]
	// vertices/indexes are not yet copied from gltf to our buffers
	Log("Meshes in " << filename.c_str() << endl);
	int modelindex = 0;
	for (auto& m : model.meshes) {
		Log("  " << m.name.c_str() << endl);
		MeshInfo* mesh = nullptr;
		if (modelindex > 0) {
			mesh = engine->meshStore.initMeshInfo(coll, coll->id + "." + to_string(modelindex));
		} else {
			mesh = coll->meshInfos[0];
		}
		mesh->gltfMeshIndex = modelindex;
        mesh->name = m.name;

		// 1) Load geometry first (creates uv0/uv1 on vertices)
		loadVertices(model, mesh, mesh->vertices, mesh->indices, modelindex);

		// 2) Then prepare textures and materials, which will parse KHR_texture_transform
		//    and bake transforms into mesh->vertices.uv0/uv1.
		prepareTexturesAndMaterials(model, coll, modelindex);

		// 3) Collect node transform
		collectBaseTransform(model, mesh);

		modelindex++;
	}
}

void glTF::mapTinyGLTFSamplerToVulkan(const tinygltf::Sampler& gltfSampler, VkSamplerCreateInfo& vkSamplerInfo) {
	vkSamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	vkSamplerInfo.magFilter = (gltfSampler.magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
	vkSamplerInfo.minFilter = (gltfSampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR) ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;

	switch (gltfSampler.wrapS) {
	case TINYGLTF_TEXTURE_WRAP_REPEAT:
		vkSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
		vkSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		break;
	case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
		vkSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		break;
	default:
		vkSamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	}

	switch (gltfSampler.wrapT) {
	case TINYGLTF_TEXTURE_WRAP_REPEAT:
		vkSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
		vkSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		break;
	case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
		vkSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		break;
	default:
		vkSamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		break;
	}

	vkSamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT; // Assuming wrapR is not used

	vkSamplerInfo.anisotropyEnable = VK_FALSE; // Set to VK_TRUE if anisotropy is needed
	vkSamplerInfo.maxAnisotropy = 1.0f; // Set to the maximum anisotropy level if anisotropy is enabled
	vkSamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	vkSamplerInfo.unnormalizedCoordinates = VK_FALSE;
	vkSamplerInfo.compareEnable = VK_FALSE;
	vkSamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	vkSamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	vkSamplerInfo.mipLodBias = 0.0f;
	vkSamplerInfo.minLod = 0.0f;
	vkSamplerInfo.maxLod = VK_LOD_CLAMP_NONE;
}
