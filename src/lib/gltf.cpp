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

void extractVertexAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const std::string& attributeName, std::vector<float>& outData, int& stride) {
	auto it = primitive.attributes.find(attributeName);
	if (it != primitive.attributes.end()) {
		const tinygltf::Accessor& accessor = model.accessors[it->second];
		const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
		const float* bufferData = reinterpret_cast<const float*>(&(model.buffers[bufferView.buffer].data[accessor.byteOffset + bufferView.byteOffset]));
		stride = accessor.ByteStride(bufferView) ? (accessor.ByteStride(bufferView) / sizeof(float)) : tinygltf::GetNumComponentsInType(accessor.type);
		outData.assign(bufferData, bufferData + accessor.count * stride);
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

void glTF::prepareTextures(tinygltf::Model& model, MeshCollection* coll, int gltfMeshIndex)
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

	// create samplers
	vector<VkSampler> samplers(model.samplers.size());
	for (int i = 0; i < model.samplers.size(); i++) {
        VkSamplerCreateInfo vkSamplerInfo{};
        mapTinyGLTFSamplerToVulkan(model.samplers[i], vkSamplerInfo);
        samplers[i] = engine->globalRendering.samplerCache.getOrCreateSampler(engine->globalRendering.device, vkSamplerInfo);
        //Log("sampler: " << model.samplers[i].name.c_str() << endl);
	}

	// texture UV coords are likely the same, but we parse their mem location and stride for all textures anyway:
	if (baseColorTextureIndex >= 0) {
		mesh->baseColorTexture = coll->textureInfos[model.textures[baseColorTextureIndex].source];
		getTextureUVCoordinates(model, primitive, mesh->baseColorTexture, mat.pbrMetallicRoughness.baseColorTexture.texCoord);
        mesh->baseColorTexture->sampler = samplers[model.textures[baseColorTextureIndex].sampler];
        mesh->baseColorTexture->type = TextureType::TEXTURE_TYPE_GLTF;
	}
	if (metallicRoughnessTextureIndex >= 0) {
		mesh->metallicRoughnessTexture = coll->textureInfos[model.textures[metallicRoughnessTextureIndex].source];
		getTextureUVCoordinates(model, primitive, mesh->metallicRoughnessTexture, mat.pbrMetallicRoughness.metallicRoughnessTexture.texCoord);
        mesh->metallicRoughnessTexture->sampler = samplers[model.textures[metallicRoughnessTextureIndex].sampler];
        mesh->metallicRoughnessTexture->type = TextureType::TEXTURE_TYPE_GLTF;
	}
	if (normalTextureIndex >= 0) {
		mesh->normalTexture = coll->textureInfos[model.textures[normalTextureIndex].source ];
		getTextureUVCoordinates(model, primitive, mesh->normalTexture, mat.normalTexture.texCoord);
        mesh->normalTexture->sampler = samplers[model.textures[normalTextureIndex].sampler];
        mesh->normalTexture->type = TextureType::TEXTURE_TYPE_GLTF;
	}
	if (occlusionTextureIndex >= 0) {
		mesh->occlusionTexture = coll->textureInfos[model.textures[occlusionTextureIndex].source];
		getTextureUVCoordinates(model, primitive, mesh->occlusionTexture, mat.occlusionTexture.texCoord);
        mesh->occlusionTexture->sampler = samplers[model.textures[occlusionTextureIndex].sampler];
        mesh->occlusionTexture->type = TextureType::TEXTURE_TYPE_GLTF;
	}
	if (emissiveTextureIndex >= 0) {
		mesh->emissiveTexture = coll->textureInfos[model.textures[emissiveTextureIndex].source];
		getTextureUVCoordinates(model, primitive, mesh->occlusionTexture, mat.emissiveTexture.texCoord);
        mesh->emissiveTexture->sampler = samplers[model.textures[emissiveTextureIndex].sampler];
        mesh->emissiveTexture->type = TextureType::TEXTURE_TYPE_GLTF;
	}
}

void glTF::validateModel(tinygltf::Model& model, MeshCollection* coll)
{
	stringstream s;
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
				s << "gltf baseColorTexture not found: " << coll->filename << ". try gltf-transform metalrough infile outfile, or mark mesh as MESH_TYPE_NO_TEXTURES to load without textures" << endl;
				Error(s.str());
			}
			texIndex = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;
			if (texIndex < 0) {
				s << "gltf metallicRoughnessTexture not found: " << coll->filename << ". try gltf-transform metalrough infile outfile, or mark mesh as MESH_TYPE_NO_TEXTURES to load without textures" << endl;
				Error(s.str());
			}
			texIndex = mat.normalTexture.index;
			if (texIndex < 0) {
				s << "gltf normalTexture not found: " << coll->filename << ". try gltf-transform metalrough infile outfile, or mark mesh as MESH_TYPE_NO_TEXTURES to load without textures" << endl;
				Log(s.str());
			}
			texIndex = mat.occlusionTexture.index;
			if (texIndex < 0) {
				s << "gltf occlusionTexture not found: " << coll->filename << ". try gltf-transform metalrough infile outfile, or mark mesh as MESH_TYPE_NO_TEXTURES to load without textures" << endl;
				Log(s.str());
			}
			texIndex = mat.emissiveTexture.index;
			if (texIndex < 0) {
				s << "gltf emissiveTexture not found: " << coll->filename << ". try gltf-transform metalrough infile outfile, or mark mesh as MESH_TYPE_NO_TEXTURES to load without textures" << endl;
				Log(s.str());
			}
		}
		if (mat.doubleSided != true) {
			Log("PERFORANCE WARNING: gltf material is single sided, but rendered doublesided! " << mat.name.c_str())
		}
	}
	for (auto& m : model.meshes) {
		//Log("  " << m.name.c_str() << endl);
		size_t size = m.primitives.size();
		//Log("  primitives: " << size << endl);
		if (size != 1) {
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

void glTF::collectBaseTransform(tinygltf::Model& model, MeshInfo* mesh)
{
	Mesh& gltfMesh = model.meshes[mesh->gltfMeshIndex];
	// collect node children to check uniqueness and have access to parents
	// map children index to parent index (parent not directly available on tinygltf::Node)
	unordered_map<int, int> childrenMap;
	childrenMap[0] = -1; // add one node parent for root

	// look for nodes for this mesh - we can only handle single reference!
	int found = -1;
	for (int i = 0; i < model.nodes.size(); i++) {
		Node& node = model.nodes[i];
		Log("node " << node.name.c_str() << endl);
		if (node.mesh == mesh->gltfMeshIndex) {
			if (found >= 0) {
				// 2nd occurence not allowed
				Error("gltf model has multiple nodes for mesh.");
			} else {
				found = i;
			}
		}
		// store children to check well formed strict tree:
		for (int child : node.children) {
			if (childrenMap.count(child) > 0) {
				//Error("Invalid node hierarchy found in gltf file");
                Log("WARNING: Invalid node hierarchy found in gltf file" << endl);
			} else {
				childrenMap[child] = i;
			}
		}
	}
	if (found == -1) {
		Log("no mesh reference found in gltf node hierarchy for " << gltfMesh.name.c_str() << " from file " << mesh->collection->filename.c_str() << endl);
		mesh->baseTransform = glm::mat4(1.0f);
	}

	// collect node transform hierarchy:
	glm::mat4 transform(1.0);
	int curIndex = found;
	do {
		Node& node = model.nodes[curIndex];
        if (node.skin > -1) {
            Error("gltf model has skinning, not supported");
        }
		if (node.scale.size() == 3) {
			glm::vec3 scaleVec(node.scale[0], node.scale[1], node.scale[2]);
			transform = glm::scale(transform, scaleVec);
		}
		if (node.rotation.size() == 4) {
			glm::quat quatVec((float)node.rotation[1], (float)node.rotation[2], (float)node.rotation[3], (float)node.rotation[0]);
			glm::mat4 rotMatrix = glm::toMat4(quatVec);
			transform = rotMatrix * transform;
		}
		curIndex = childrenMap[curIndex];
	} while (curIndex != -1);
	mesh->baseTransform = transform;
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
		prepareTextures(model, coll, modelindex);
		loadVertices(model, mesh, mesh->vertices, mesh->indices, modelindex);
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
