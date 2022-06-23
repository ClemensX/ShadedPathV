#include "pch.h"

void ObjectStore::init(ShadedPathEngine* engine) {
	this->engine = engine;
}

ObjectInfo* ObjectStore::getObject(string id)
{
	ObjectInfo*ret = &objects[id];
	// simple validity check for now:
	if (ret->id.size() > 0) {
		// if there is no id the texture could not be loaded (wrong filename?)
		ret->available = true;
	}
	else {
		ret->available = false;
	}
	return ret;
}
ObjectInfo* ObjectStore::loadObjectFile(string filename, string id, vector<byte>& fileBuffer)
{
	ObjectInfo initialObject;  // only used to initialize struct in texture store - do not access this after assignment to store

	initialObject.id = id;
	objects[id] = initialObject;
	ObjectInfo* objInfo = &objects[id];

	// find texture file, look in pak file first:
	PakEntry* pakFileEntry = nullptr;
	pakFileEntry = engine->files.findFileInPak(filename.c_str());
	// try file system if not found in pak:
	initialObject.filename = filename; // TODO check: field not needed? only in this method? --> remove
	string binFile;
	if (pakFileEntry == nullptr) {
		binFile = engine->files.findFile(filename.c_str(), FileCategory::MESH);
		objInfo->filename = binFile;
		//initialTexture.filename = binFile;
		engine->files.readFile(objInfo->filename.c_str(), fileBuffer, FileCategory::MESH);
	}
	else {
		engine->files.readFile(pakFileEntry, fileBuffer, FileCategory::MESH);
	}
	return objInfo;
}

void ObjectStore::loadObjectWireframe(string filename, string id, vector<LineDef> &lines)
{
	vector<byte> file_buffer;
	auto obj = loadObjectFile(filename, id, file_buffer);
	string fileAndPath = obj->filename;
	vector<vec3> vertices;
	vector<uint32_t> indexBuffer;
	glTF::loadVertices((const unsigned char*)file_buffer.data(), (int)file_buffer.size(), vertices, indexBuffer, fileAndPath);
	if (vertices.size() > 0) {
		for (uint32_t i = 0; i < indexBuffer.size(); i += 3) {
			// triangle i --> i+1 --> i+2
			LineDef l;
			l.color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			auto& p0 = vertices[indexBuffer[i]];
			auto& p1 = vertices[indexBuffer[i+1]];
			auto& p2 = vertices[indexBuffer[i+2]];
			l.start = p0;
			l.end = p1;
			lines.push_back(l);
			l.start = p1;
			l.end = p2;
			lines.push_back(l);
			l.start = p2;
			l.end = p0;
			lines.push_back(l);
		}
	}
	obj->available = true;
}

void ObjectStore::loadObject(string filename, string id)
{
	vector<byte> file_buffer;
	auto obj = loadObjectFile(filename, id, file_buffer);
	string fileAndPath = obj->filename;
	glTF::loadVertices((const unsigned char*)file_buffer.data(), (int)file_buffer.size(), obj->vertices, obj->indices, fileAndPath);
	obj->available = true;
}

void ObjectStore::uploadObject(ObjectInfo* obj)
{
	assert(obj->vertices.size() > 0);
	assert(obj->indices.size() > 0);

	// upload vec3 vertex buffer:
	size_t vertexBufferSize = obj->vertices.size() * sizeof(PBRShader::Vertex);
	size_t indexBufferSize = obj->indices.size() * sizeof(obj->indices[0]);
	engine->global.uploadBuffer(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize, obj->vertices.data(), obj->vertexBuffer, obj->vertexBufferMemory);
	engine->global.uploadBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBufferSize, obj->vertices.data(), obj->indexBuffer, obj->indexBufferMemory);
	engine->util.debugNameObjectBuffer(obj->vertexBuffer, "GLTF object vertex buffer");
	engine->util.debugNameObjectDeviceMmeory(obj->vertexBufferMemory, "GLTF object vertex buffer device mem");
	engine->util.debugNameObjectBuffer(obj->indexBuffer, "GLTF object index buffer");
	engine->util.debugNameObjectDeviceMmeory(obj->indexBufferMemory, "GLTF object index buffer device mem");
}

const vector<ObjectInfo*> &ObjectStore::getSortedList()
{
	if (sortedList.size() == objects.size()) {
		return sortedList;
	}
	// create list, TODO sorting
	sortedList.clear();
	sortedList.reserve(objects.size());
	for (auto& kv : objects) {
		sortedList.push_back(&kv.second);
	}
	return sortedList;
}

ObjectStore::~ObjectStore()
{
	for (auto& mapobj : objects) {
		if (mapobj.second.available) {
			auto& obj = mapobj.second;
			vkDestroyBuffer(engine->global.device, obj.vertexBuffer, nullptr);
			vkFreeMemory(engine->global.device, obj.vertexBufferMemory, nullptr);
			vkDestroyBuffer(engine->global.device, obj.indexBuffer, nullptr);
			vkFreeMemory(engine->global.device, obj.indexBufferMemory, nullptr);
		}
	}

}

