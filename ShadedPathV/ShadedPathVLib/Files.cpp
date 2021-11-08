#include "pch.h"

#if defined(DEBUG) || defined(_DEBUG)
#define FX_PATH "..\\x64\\Debug\\"
#else
#define FX_PATH "..\\x64\\Release\\"
#endif

#define TEXTURE_PATH "..\\..\\data\\texture\\"
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

