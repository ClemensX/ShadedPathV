#include "pch.h"

#if defined(DEBUG) || defined(_DEBUG)
#define FX_PATH "Debug"
#else
#define FX_PATH "Release"
#endif

using namespace std;

void Files::findFxFolder()
{
	filesystem::path current = filesystem::current_path();
	string fx_test_filename = "line_vert.spv";

	// first look at CD:
	filesystem::path to_check = current / fx_test_filename;
	Log("Looking for shader files: " << to_check << endl);
	if (filesystem::exists(to_check)) {
		fxFolder = current;
		Log("Found shader files: " << fxFolder << endl);
		return;
	}

	// then look at rel path (used from running inside VS)
	current = current / ".." / "x64" / FX_PATH;
	to_check =  current / fx_test_filename;
	if (filesystem::exists(to_check)) {
		fxFolder = current;
		Log("Found shader files: " << fxFolder << endl);
		return;
	}
	Error("shader files not found");
}

void Files::findAssetFolder(string folderName)
{
	filesystem::path current = filesystem::current_path();
	for (;;) {
		filesystem::path to_check = current / folderName;
		//Log("Looking for asset folder: " << to_check << endl);
		if (filesystem::exists(to_check)) {
			assetFolder = to_check;
			Log("Found asset folder: " << assetFolder << endl);
			return;
		}
		// if parent is identical we are at root
		if (current.compare(current.parent_path()) == 0) {
			stringstream s;
			s << "AssetFolder not found: " << folderName << endl;
			Error(s.str().c_str());
		}
		current = current.parent_path();
	}
}

string Files::findFile(string filename, FileCategory cat, bool errorIfNotFound, bool generateFilenameMode) {
	if (assetFolder.empty()) {
		Error("AssetFolder not set - cannot read any files!");
	}
	if (fxFolder.empty()) {
		Error("FxFolder not set - cannot read shader files!");
	}
	filesystem::path asset_path;
	switch (cat) {
	case FileCategory::FX:
		asset_path = fxFolder / filename;
		break;
	case FileCategory::TEXTURE:
	case FileCategory::TEXTUREPAK:
		asset_path = assetFolder / TEXTURE_PATH / filename;
		break;
	case FileCategory::MESH:
		asset_path = assetFolder / MESH_PATH / filename;
		break;
	case FileCategory::SOUND:
		asset_path = assetFolder / SOUND_PATH / filename;
		break;
	}
	if (generateFilenameMode) {
		return filename.c_str();
	}
	ifstream bfile(asset_path.c_str(), ios::in | ios::binary);
	if (!bfile && cat == FileCategory::TEXTURE) {
		string oldname = filename;
		// try loading default texture
		asset_path = assetFolder / TEXTURE_PATH / "debug.ktx";
		bfile.open(asset_path.c_str(), ios::in | ios::binary);
		if (bfile) {
			filesystem::path p = filename.c_str();
			Log("WARNING: texture " << filesystem::absolute(p) << " not found, replaced by default.dds texture" << endl);
		}

	}
	if (!bfile && errorIfNotFound) {
		filesystem::path p = filename.c_str();
		stringstream s;
		//s << "failed reading file: " << filesystem::absolute(p) << endl;
		s << "failed reading file: " << filesystem::absolute(asset_path) << endl;
		//s << "failed reading file: " << asset_path << endl;
		Error(s.str());
	}
	if (bfile) {
		bfile.close();
		return asset_path.generic_string();
	}
	return string();
}

bool Files::checkFileForWrite(string filename)
{
	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (file.fail()) {
		Log("could not open file for write " << filesystem::absolute(filename) << endl);
		file.close();
		return false;
	}
	file.close();
	return true;
}

void Files::readFile(string filename, vector<byte>& buffer, FileCategory cat) {
	filename = findFile(filename, cat);
	ifstream bfile(filename.c_str(), ios::in | ios::binary);
	if (!bfile) {
		stringstream s;
		s << "failed reading file: " << filename << endl;
		Error(s.str());
	}
	else {
		streampos start = bfile.tellg();
		bfile.seekg(0, std::ios::end);
		streampos len = bfile.tellg() - start;
		bfile.seekg(start);
		buffer.resize((SIZE_T)len);
		bfile.read((char*)&(buffer[0]), len);
		bfile.close();
	}

}

void Files::writeFile(string filename, const char* buf, int size) {
	ofstream bfile(filename.c_str(), ios::out | ios::binary);
	if (!bfile) {
		stringstream s;
		s << "failed writing file: " << filename << endl;
		Error(s.str());
	}
	else {
		bfile.write((char*)(buf), size);
		bfile.close();
	}

}

void Files::readFile(PakEntry* pakEntry, vector<byte>& buffer, FileCategory cat)
{
	Log("read file from pak: " << pakEntry->name.c_str() << endl);
	ifstream bfile(pakEntry->pakname.c_str(), ios::in | ios::binary);
	if (!bfile) {
		stringstream s;
		s << "failed re-opening pak file: " << pakEntry->pakname << endl;
		Error(s.str());
	}
	else {
		// position to start of file in pak:
		bfile.seekg(pakEntry->offset);
		buffer.resize(pakEntry->len);
		bfile.read((char*)&(buffer[0]), pakEntry->len);
		bfile.close();
	}
}

PakEntry* Files::findFileInPak(string filename)
{
	auto gotit = pak_content.find(filename);
	if (gotit == pak_content.end()) {
		return nullptr;
	}
	return &gotit->second;
	//if (pak_content.count(name) == 0) {
	//	return nullptr;
	//}
	//PakEntry *pe = &pak_content[name];
	//return pe;
}

string Files::absoluteFilePath(string filename)
{
	string absfile(filesystem::absolute(filename).string());
	return absfile;
}
