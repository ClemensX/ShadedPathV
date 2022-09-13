#pragma once
class WorldObject;

enum SoundCategory { MUSIC, EFFECT };

// miniaudio forward declaration:
struct ma_sound;

struct SoundDef {
	bool loop = false;
	SoundCategory category;
	// miniaudio entries:
	ma_sound* masound = nullptr;
};


class Sound
{
public:
	Sound(ShadedPathEngine& s) : engine(s) {
		Log("Sound c'tor\n");
	}
	~Sound(void);
	void init();
	int addWorldObject(WorldObject* wo, char *cueName);
	std::unordered_map<std::string, SoundDef> sounds;
	// engine jingle:
	inline static const std::string SHADED_PATH_JINGLE_FILE = "shaded_path_jingle.ogg";
	inline static const std::string SHADED_PATH_JINGLE = "shaded_path_jingle";
private:
	std::wstring project_filename;
	std::vector<WorldObject*> audibleWorldObjects;  // index used instead of passing WorldObject down to sound class

	int numDoNothingFrames = 0;
	bool recalculateSound();
	ShadedPathEngine& engine;
	// only run methods if initialized - immediately return otherwise
	bool initialized = false;
public:
	void Update();
	void openSoundFile(std::string soundFileName, std::string id, bool loop = false);
	// play sound with possible delay (in ms) and volume
	void playSound(std::string id, SoundCategory category = EFFECT, float volume = 1.0f, uint32_t delayMS = 0);
	void lowBackgroundMusicVolume(bool volumeDown = true);
};
