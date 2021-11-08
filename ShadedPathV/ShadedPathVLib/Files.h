#pragma once

// all files hav an associated category
enum class FileCategory { FX, TEXTURE, MESH, SOUND, TEXTUREPAK };

// File handling: all file types that need to be read or written at runtime go through here
class Files
{
public:
	// find absolute filename for a name and category, defaults to display error dialog, returns empty filename if not found and errorIfNotFound is set to false,
	// returns full file path if generateFilenameMode == true (use to create files)
	string findFile(string filename, FileCategory cat, bool errorIfNotFound = true, bool generateFilenameMode = false);
	//string findFileForCreation(string filename, FileCategory cat) { return findFile(filename, cat, false, true); };
	void readFile(string filename, vector<byte>& buffer, FileCategory cat);
	//void readFile(PakEntry* pakEntry, vector<byte>& buffer, FileCategory cat);
	//PakEntry* findFileInPak(wstring filename);
	void initPakFiles();
	static void logThreadInfo(string info);
};

