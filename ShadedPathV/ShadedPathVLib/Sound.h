#pragma once
class WorldObject;

enum SoundCategory { MUSIC, EFFECT };

struct SoundDef {
	bool loop = false;
	SoundCategory category;
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
private:
	std::wstring project_filename;
	std::vector<WorldObject*> audibleWorldObjects;  // index used instead of passing WorldObject down to sound class

	int numDoNothingFrames;
	bool recalculateSound();
	ShadedPathEngine& engine;
	// only run methods if initialized - immediately return otherwise
	bool initialized = false;
public:
	void Update();
	void openSoundFile(std::wstring soundFileName, std::string id, bool loop = false);
	void playSound(std::string id, SoundCategory category = EFFECT, float volume = 1.0f);
	void lowBackgroundMusicVolume(bool volumeDown = true);
};
