#include "pch.h"

#if defined(DEBUG) || defined(_DEBUG)
#define FX_PATH "..\\x64\\Debug\\"
#else
#define FX_PATH "..\\x64\\Release\\"
#endif

#define TEXTURE_PATH "..\\..\\..\\data\\texture\\"
#define MESH_PATH "..\\..\\data\\mesh\\"
#define SOUND_PATH "..\\..\\data\\sound\\"

string Files::findFile(string filename, FileCategory cat, bool errorIfNotFound, bool generateFilenameMode) {
	// try without path:
	ifstream bfile(filename.c_str(), ios::in | ios::binary);
	if (!bfile) {
		// try with Debug or release path:
		switch (cat) {
		case FileCategory::FX:
			filename = FX_PATH + filename;
			break;
		case FileCategory::TEXTURE:
		case FileCategory::TEXTUREPAK:
			filename = TEXTURE_PATH + filename;
			break;
		case FileCategory::MESH:
			filename = MESH_PATH + filename;
			break;
		case FileCategory::SOUND:
			filename = SOUND_PATH + filename;
			break;
		}
		if (generateFilenameMode) {
			return filename.c_str();
		}
		bfile.open(filename.c_str(), ios::in | ios::binary);
		if (!bfile && cat == FileCategory::TEXTURE) {
			string oldname = filename;
			// try loading default texture
			filename = TEXTURE_PATH + string("default.dds");
			bfile.open(filename.c_str(), ios::in | ios::binary);
			if (bfile) {
				filesystem::path p = filename.c_str();
				Log("WARNING: texture " << filesystem::absolute(p) << " not found, replaced by default.dds texture" << endl);
			}

		}
		if (!bfile && errorIfNotFound) {
			filesystem::path p = filename.c_str();
			stringstream s;
			s << "failed reading file: " << filesystem::absolute(p) << endl;
			Error(s.str());
		}
	}
	if (bfile) {
		bfile.close();
		return filename;
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
