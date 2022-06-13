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

void ObjectStore::loadObject(string filename, string id, vector<LineDef> &lines)
{
	ObjectInfo initialObject;  // only used to initialize struct in texture store - do not access this after assignment to store
	vector<byte> file_buffer;

	initialObject.id = id;
	objects[id] = initialObject;
	ObjectInfo*texture = &objects[id];

	// find texture file, look in pak file first:
	PakEntry *pakFileEntry = nullptr;
	pakFileEntry = engine->files.findFileInPak(filename.c_str());
	// try file system if not found in pak:
	initialObject.filename = filename; // TODO check: field not needed? only in this method? --> remove
	string binFile;
	if (pakFileEntry == nullptr) {
		binFile = engine->files.findFile(filename.c_str(), FileCategory::MESH);
		texture->filename = binFile;
		//initialTexture.filename = binFile;
		engine->files.readFile(texture->filename.c_str(), file_buffer, FileCategory::MESH);
	} else {
		engine->files.readFile(pakFileEntry, file_buffer, FileCategory::MESH);
	}
	vector<vec3> vertices;
	vector<uint32_t> indexBuffer;
	glTF::loadVertices((const unsigned char*)file_buffer.data(), file_buffer.size(), vertices, indexBuffer, binFile);
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

		//for (size_t i = 0; i < vertices.size()-1; i++) {
		//	LineDef l;
		//	l.color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		//	l.start = vertices[i];
		//	l.end = vertices[i + 1];
		//	lines.push_back(l);
		//}
	}
	texture->available = true;
}

ObjectStore::~ObjectStore()
{
	for (auto& tex : objects) {
		if (tex.second.available) {
			// nothing to do (yet)
		}
	}

}

