 //Matthew Rosen
#pragma once

#include "Util.h"
#include <portaudio.h>
#include <Planeverb.h>

struct AudioData;

struct PlayingData
{
	bool currentlyPlaying = false;
	bool usePlaneverb = false;
	float volume = 1.f;
	int readIndex = 0;
	AudioData* dataPlaying = nullptr;
	Planeverb::EmissionID id = Planeverb::PV_INVALID_EMISSION_ID;
};

float gainToDB(float gain);
float dBToGain(float dB);

class AudioCore : public Singleton<AudioCore>
{
public:
	void Init();
	void Update(const Planeverb::vec3& pos);
	void Exit();

	void StopAudio();
	Planeverb::EmissionID PlayAudio(const Planeverb::vec3& emitter);

	float& GetVolume();
	void SetVolume(float gain);

	void SetAudioData(AudioData* adata);
	void SetUsePlaneverb(bool use) { m_data.usePlaneverb = use; }

	void ProcessBlock(float* out, int frames);
private:
	PlayingData m_data;
	PaStream* m_stream;
};
