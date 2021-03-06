#pragma once

// all files hav an associated category
enum class FileCategory { FX, TEXTURE, MESH, SOUND, TEXTUREPAK };

class PakEntry {
public:
	long len;    // file length in bytes
	long offset; // offset in pak - will be transferred to absolute
				 // position in pak file on save
	string name; // directory entry - may contain fake folder names
				 // 'sub/t.dds'
	//ifstream *pakFile; // reference to pak file, stream should be open and ready to read at all times
	string pakname; // we open and close the pak file for every read, so we store filename here
};

// File handling: all file types that need to be read or written at runtime go through here
class Files
{
public:
	// set asset folder name - the folder will be looked for up the whole path, beginning with CD of executable. First match wins, Error if not found at all
	void findAssetFolder(string folderName);
	// find folder with compiled shaders
	void findFxFolder();
	// find absolute filename for a name and category, defaults to display error dialog, returns empty filename if not found and errorIfNotFound is set to false,
	// returns full file path if generateFilenameMode == true (use to create files)
	string findFile(string filename, FileCategory cat, bool errorIfNotFound = true, bool generateFilenameMode = false);
	string findFileForCreation(string filename, FileCategory cat) { return findFile(filename, cat, false, true); };
	void readFile(string filename, vector<byte>& buffer, FileCategory cat);
	// dump memory to file
	void writeFile(string filename, const char* buf, int size);
	// check file can be opened for write operation. Logs error if not.
	bool checkFileForWrite(string filename);
	// get absolute file path
	string absoluteFilePath(string filename);
	void readFile(PakEntry* pakEntry, vector<byte>& buffer, FileCategory cat);
	PakEntry* findFileInPak(string filename);
	void initPakFiles();
	static void logThreadInfo(string info);
private:
	// pak files:
	unordered_map<string, PakEntry> pak_content;

	filesystem::path assetFolder;
	filesystem::path fxFolder;

	//define sub folder names, all directly below asset folder
	const string TEXTURE_PATH = "texture";
	const string MESH_PATH = "mesh";
	const string SOUND_PATH = "sound";
};

