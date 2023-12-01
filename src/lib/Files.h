#pragma once

// all files hav an associated category
enum class FileCategory { FX, TEXTURE, MESH, SOUND, TEXTUREPAK };

class PakEntry {
public:
	long len;    // file length in bytes
	long offset; // offset in pak - will be transferred to absolute
				 // position in pak file on save
	std::string name; // directory entry - may contain fake folder names
				 // 'sub/t.dds'
	//ifstream *pakFile; // reference to pak file, stream should be open and ready to read at all times
	std::string pakname; // we open and close the pak file for every read, so we store filename here
};

// File handling: all file types that need to be read or written at runtime go through here
class Files
{
public:
	// set asset folder name - the folder will be looked for up the whole path, beginning with CD of executable. First match wins, Error if not found at all
	void findAssetFolder(std::string folderName);
	// find folder with compiled shaders
	void findFxFolder();
	// find absolute filename for a name and category, defaults to display error dialog, returns empty filename if not found and errorIfNotFound is set to false,
	// returns full file path if generateFilenameMode == true (use to create files)
	std::string findFile(std::string filename, FileCategory cat, bool errorIfNotFound = true, bool generateFilenameMode = false);
	std::string findFileForCreation(std::string filename, FileCategory cat) { return findFile(filename, cat, false, true); };
	void readFile(std::string filename, std::vector<std::byte>& buffer, FileCategory cat);
	// dump memory to file
	void writeFile(std::string filename, const char* buf, int size);
	// check file can be opened for write operation. Logs error if not.
	bool checkFileForWrite(std::string filename);
	// get absolute file path
	std::string absoluteFilePath(std::string filename);
	void readFile(PakEntry* pakEntry, std::vector<std::byte>& buffer, FileCategory cat);
	PakEntry* findFileInPak(std::string filename);
	static void logThreadInfo(std::string info);
private:
	// initiate pak files before any other file operation. Called from findAssetFolder()
	void initPakFiles();
	// pak files:
	std::unordered_map<std::string, PakEntry> pak_content;

	std::filesystem::path assetFolder;
	std::filesystem::path fxFolder;

	//define sub folder names, all directly below asset folder
	const std::string TEXTURE_PATH = "texture";
	const std::string MESH_PATH = "mesh";
	const std::string SOUND_PATH = "sound";
};
