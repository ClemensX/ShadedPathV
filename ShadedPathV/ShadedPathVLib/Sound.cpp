#include "pch.h"
//#define STB_VORBIS_HEADER_ONLY
//#include "miniaudio/extras/stb_vorbis.c"    // Enables Vorbis decoding.
//
//#define MINIAUDIO_IMPLEMENTATION
//#include "miniaudio/miniaudio.h"
//
//// The stb_vorbis implementation must come after the implementation of miniaudio.
//#undef STB_VORBIS_HEADER_ONLY
//#include "miniaudio/extras/stb_vorbis.c"

// always assume 2 source channels, TODO ? check
#define SRC_CHANNEL_COUNT 2

void Sound::init()
{
	numDoNothingFrames = 1;
	HRESULT hr;
	UINT32 flags = 0;
#if defined (_DEBUG)
	//flags |= XAUDIO2_DEBUG_ENGINE;
#endif
	//UINT32 count;
	//xaudio2->GetDeviceCount(&count);
	//Log("xaudio2 device count == " << count << endl);
	// use decice 0 (== global default sudio device)

	// setup DSP

	// setup emitter and listener
}

Sound::~Sound(void)
{
	for (auto s : sounds) {
		//if (s.second.voice) {
		//	s.second.voice->DestroyVoice();
		//}
		//delete s.second.buffer.pAudioData;
	}
}

void Sound::Update() {
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

VOID Sound::openSoundFile(wstring fileName, string id, bool loop)
{
	HWND hWnd = nullptr;//DXUTGetHWND();
	//HRESULT hr;
	wstring binFile; // = xapp().findFile(fileName.c_str(), XApp::SOUND);
	SoundDef sd;
	if (sounds.count(id) == 0) {
		ZeroMemory(&sd, sizeof(sd));
	} else {
		sd = sounds[id];
	}
	sd.loop = loop;
	//openSoundWithXAudio2(sd, binFile);
	sounds[id] = sd;
	return;
}

void Sound::playSound(string id, SoundCategory category, float volume) {
	HRESULT hr;
	assert(sounds.count(id) > 0);
	SoundDef *sound = &sounds[id];
	sound->category = category;
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

int Sound::addWorldObject(WorldObject* wo, char *cueName) {
	audibleWorldObjects.push_back(wo);
	return -1;
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