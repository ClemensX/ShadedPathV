#include "pch.h"

// remove our CHECK macro as stb_vorbis.c defines it's own
#undef CHECK
#define STB_VORBIS_HEADER_ONLY
#include "miniaudio/extras/stb_vorbis.c"    // Enables Vorbis decoding.

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

// The stb_vorbis implementation must come after the implementation of miniaudio.
#undef STB_VORBIS_HEADER_ONLY
#include "miniaudio/extras/stb_vorbis.c"

// always assume 2 source channels, TODO ? check
//#define SRC_CHANNEL_COUNT 2

ma_result result;
ma_engine sound_engine;

void Sound::init()
{
	numDoNothingFrames = 1;
	HRESULT hr;
	UINT32 flags = 0;
#if defined (_DEBUG)
	//flags |= XAUDIO2_DEBUG_ENGINE;
#endif
	// init minaudio high level API engine
	result = ma_engine_init(NULL, &sound_engine);
	if (result != MA_SUCCESS) {
		Log("Sound unavailable. Disabling.");
		engine.enableSound(false);
		return;  // Failed to initialize the engine.
	}
	Log("Sound initialized" << std::endl);
	openSoundFile(Sound::SHADED_PATH_JINGLE_FILE, Sound::SHADED_PATH_JINGLE);
	playSound(Sound::SHADED_PATH_JINGLE, SoundCategory::MUSIC);
}

Sound::~Sound(void)
{
	Log("Sound d'tor" << std::endl);
	if (!engine.isSoundEnabled()) return;
	for (auto s : sounds) {
		ma_sound_uninit(s.second.masound);
		delete s.second.masound;
		s.second.masound = nullptr;
		//if (s.second.voice) {
		//	s.second.voice->DestroyVoice();
		//}
		//delete s.second.buffer.pAudioData;
	}
	ma_engine_uninit(&sound_engine);
}

void Sound::changeSound(WorldObject* wo, std::string soundId)
{
	assert(sounds.count(soundId) > 0);
	SoundDef* sound = &sounds[soundId];
	if (wo->playing) {
		// TODO stop sound
	}
	wo->soundDef = sound;
	playSound(soundId, SoundCategory::EFFECT);
}

void Sound::Update(glm::vec3& pos, glm::vec3& lookAt) {
	if (!engine.isSoundEnabled()) return;
	// set listener pos
	ma_engine_listener_set_position(&sound_engine, 0, pos.x, pos.y, pos.z);
	ma_engine_listener_set_direction(&sound_engine, 0, lookAt.x, lookAt.y, lookAt.z);
	// adjust sound positions
	for (int i = 0; i < audibleWorldObjects.size(); i++) {
		WorldObject* wo = audibleWorldObjects[i];
		if (wo->soundDef == nullptr) {
			continue;  // nothing to play right now
		} else {
			// adjust positioning
			ma_sound_set_position(wo->soundDef->masound, wo->pos().x, wo->pos().y, wo->pos().z);
		}
	}
	//Camera* cam = &xapp().camera;;
	////HRESULT hr;
	//if (cam && recalculateSound()) {
	//	// update 3d sounds
	//	listener.OrientFront.x = cam->look.x;
	//	listener.OrientFront.y = cam->look.y;
	//	listener.OrientFront.z = cam->look.z;
	//	listener.OrientTop.x = cam->up.x;
	//	listener.OrientTop.y = cam->up.y;
	//	listener.OrientTop.z = cam->up.z;
	//	listener.Position.x = cam->pos.x;
	//	listener.Position.y = cam->pos.y;
	//	listener.Position.z = cam->pos.z;
	//	for (int i = 0; i < audibleWorldObjects.size(); i++) {
	//		WorldObject *wo = audibleWorldObjects[i];
	//		if (wo->soundDef == nullptr) {
	//			continue;  // nothing to play right now
	//		}
	//		emitter.Position.x = wo->pos().x;
	//		emitter.Position.y = wo->pos().y;
	//		emitter.Position.z = wo->pos().z;
	//		emitter.ChannelCount = wo->soundDef->wfx.Format.nChannels;
	//		dspSettings.SrcChannelCount = emitter.ChannelCount;
	//		//emitter.pChannelAzimuths[0] = 4.71f;
	//		//emitter.pChannelAzimuths[1] = 1.57f;
	//		X3DAudioCalculate(x3dInstance, &listener, &emitter, X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER | X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_REVERB, &dspSettings);
	//		float distance = dspSettings.EmitterToListenerDistance;
	//		float maxDistance = (float)wo->maxListeningDistance;
	//		//Log(" distance " << dspSettings.EmitterToListenerDistance << "\n");
	//		SoundDef *s = wo->soundDef;
	//		if (true || !wo->stopped) {
	//			s->voice->SetFrequencyRatio(dspSettings.DopplerFactor);
	//			s->voice->SetOutputMatrix(submixVoiceEffect, emitter.ChannelCount, dspSettings.DstChannelCount, dspSettings.pMatrixCoefficients);
	//			XAUDIO2_FILTER_PARAMETERS FilterParameters = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * dspSettings.LPFDirectCoefficient), 1.0f };
	//			float current_volume;
	//			submixVoiceEffect->GetVolume(&current_volume);
	//			submixVoiceEffect->SetVolume(current_volume);
	//		}
	//		/*if (!wo->playing) {
	//		V(wo->cue->Play());
	//		wo->playing = true;
	//		}
	//		if (!wo->stopped && distance > maxDistance) {
	//		// moved outside listening distance
	//		wo->cue->Pause(true);
	//		wo->stopped = true;
	//		oss << " stop \n";
	//		Blender::Log(oss.str());
	//		}
	//		else if (wo->stopped && distance <= maxDistance) {
	//		// moved inside listening distance
	//		wo->cue->Pause(false);
	//		wo->stopped = false;
	//		oss << " run \n";
	//		Blender::Log(oss.str());
	//		}*/

	//	}
	//}
}

#ifndef _XBOX //Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

VOID Sound::openSoundFile(std::string fileName, std::string id, bool loop)
{
	if (engine.isRendering()) {
		Log("WARNING: do not load sound files during rendering!");
	}
	std::string binFile;
	binFile = engine.files.findFile(fileName.c_str(), FileCategory::SOUND);
	SoundDef sd;
	if (sounds.count(id) == 0) {
		ZeroMemory(&sd, sizeof(sd));
	} else {
		sd = sounds[id];
	}
	sd.loop = loop;
	sounds[id] = sd;
	if (!engine.isSoundEnabled()) return;
	ma_sound *m = new ma_sound;
	sounds[id].masound = m;
	auto mas = sounds[id].masound; // mas is a ptr
	result = ma_sound_init_from_file(&sound_engine, binFile.c_str(), NULL, NULL, NULL, mas);
	if (result != MA_SUCCESS) {
		Error("Could not load sound");
	}
	return;
}

void Sound::playSound(std::string id, SoundCategory category, float volume, uint32_t delayMS) {
	if (!engine.isSoundEnabled()) return;
	//result = ma_engine_play_sound(&sound_engine, "C:\\\\dev\\vulkan\\data\\sound\\Free_Test_Data_100KB_OGG.ogg", NULL);
	//if (result != MA_SUCCESS) {
	//	Log("Cannot play sound " << result << std::endl);
	//}
	HRESULT hr;
	assert(sounds.count(id) > 0);
	SoundDef *sound = &sounds[id];
	sound->category = category;
	if (sound->loop) {
		ma_sound_set_looping(sound->masound, true);
	}
	if (delayMS > 0) {
		uint32_t delaySamples = ma_engine_get_sample_rate(&sound_engine) * delayMS / 1000;
		ma_sound_set_start_time_in_pcm_frames(sound->masound, ma_engine_get_time(&sound_engine) + delaySamples);
	}
	if (category == MUSIC) {
		ma_sound_set_spatialization_enabled(sound->masound, false);
	}
	ma_sound_start(sound->masound);
	//XAUDIO2_VOICE_SENDS *sendsList = category == MUSIC ? sfxSendsListMusic : sfxSendsListEffect;
	//hr = xaudio2->CreateSourceVoice(&sound->voice, (WAVEFORMATEX*)&sound->wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, nullptr, sendsList, nullptr);
	//ThrowIfFailed(hr);
	//hr = sound->voice->SubmitSourceBuffer(&sound->buffer);
	//ThrowIfFailed(hr);
	//sound->voice->SetVolume(volume); // TODO cleanup
	//hr = sound->voice->Start(0);
	//float current_volume;
	//sound->voice->GetVolume(&current_volume);
	if (category == MUSIC) {
		//sound->voice->SetVolume(current_volume/15.0f); // TODO cleanup
	}
	//ThrowIfFailed(hr);
}

void Sound::lowBackgroundMusicVolume(bool volumeDown) {
	//SoundDef *sound = &sounds[id];
	if (volumeDown) {
//		this->submixVoiceBackground->SetVolume(0.5f);
	} else {
//		this->submixVoiceBackground->SetVolume(1.0f);
	}
}

void Sound::addWorldObject(WorldObject* wo) {
	audibleWorldObjects.push_back(wo);
}

#define DO_NOTHING_FRAME_COUNT 5

bool Sound::recalculateSound() {
	if (numDoNothingFrames <= 0) {
		numDoNothingFrames = DO_NOTHING_FRAME_COUNT;
		return true;
	} else {
		numDoNothingFrames--;
		return false;
	}
}

//XApp* Sound::xapp = nullptr;