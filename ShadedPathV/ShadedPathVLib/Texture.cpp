#include "pch.h"

void TextureStore::init(ShadedPathEngine* engine) {
	this->engine = engine;
}

TextureInfo* TextureStore::getTexture(string id)
{
	TextureInfo *ret = &textures[id];
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

void TextureStore::loadTexture(string filename, string id)
{
	TextureInfo initialTexture;  // only used to initialize struct in texture store - do not access this after assignment to store
	vector<byte> file_buffer;

	initialTexture.id = id;
	textures[id] = initialTexture;
	TextureInfo *texture = &textures[id];

	// find texture file, look in pak file first:
	PakEntry *pakFileEntry = nullptr;
	pakFileEntry = engine->files.findFileInPak(filename.c_str());
	// try file system if not found in pak:
	initialTexture.filename = filename; // TODO check: field not needed? only in this method? --> remove
	if (pakFileEntry == nullptr) {
		string binFile = engine->files.findFile(filename.c_str(), FileCategory::TEXTURE);
		texture->filename = binFile;
		//initialTexture.filename = binFile;
		engine->files.readFile(texture->filename.c_str(), file_buffer, FileCategory::TEXTURE);
	} else {
		engine->files.readFile(pakFileEntry, file_buffer, FileCategory::TEXTURE);
	}

}
