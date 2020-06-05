//Matthew Rosen

#include "AudioCore.h"
#include "AudioData.h"
#include <PlaneverbDSP.h>
#include <cmath>

float gainToDB(float gain)
{
	return 20.f * std::log10(gain);
}

float dBToGain(float dB)
{
	return std::pow(10.f, dB / 20.f);
}

static int OnWrite(const void* vin, void* vout, unsigned long frames,
	const PaStreamCallbackTimeInfo* tinfo,
	PaStreamCallbackFlags flags, void* user)
{
	float* out = reinterpret_cast<float*>(vout);
	AudioCore* core = reinterpret_cast<AudioCore*>(user);
	core->ProcessBlock(out, (int)frames);
	return paContinue;
}

void AudioCore::Init()
{
	Pa_Initialize();

	PaStreamParameters output_params;
	output_params.device = Pa_GetDefaultOutputDevice();
	output_params.channelCount = CHANNELS;
	output_params.sampleFormat = paFloat32;
	output_params.suggestedLatency = Pa_GetDeviceInfo(output_params.device)->defaultHighOutputLatency;
	output_params.hostApiSpecificStreamInfo = 0;

	Pa_OpenStream(&m_stream, 0, &output_params, (double)RATE, FRAMES_PER_BLOCK, 0,
		OnWrite, this);
	Pa_StartStream(m_stream);
}

void AudioCore::Update(const Planeverb::vec3& pos)
{
	if (m_data.id != Planeverb::PV_INVALID_EMISSION_ID)
	{
		Planeverb::UpdateEmission(m_data.id, pos);
	}
}

void AudioCore::Exit()
{
	StopAudio();
	Pa_StopStream(m_stream);
	Pa_CloseStream(m_stream);
	Pa_Terminate();
}

void AudioCore::StopAudio()
{
	m_data.currentlyPlaying = false;
	Planeverb::EndEmission(m_data.id);
	m_data.id = Planeverb::PV_INVALID_EMISSION_ID;
}

Planeverb::EmissionID AudioCore::PlayAudio(const Planeverb::vec3& emitter)
{
	m_data.currentlyPlaying = true;
	m_data.readIndex = 0;
	m_data.id = Planeverb::Emit(emitter);
	return m_data.id;
}

float & AudioCore::GetVolume()
{
	return m_data.volume;
}

void AudioCore::SetVolume(float gain)
{
	m_data.volume = gain;
}

void AudioCore::SetAudioData(AudioData * adata)
{
	m_data.dataPlaying = adata;
}

void AudioCore::ProcessBlock(float * out, int frames)
{
	int samples = frames * CHANNELS;
	std::memset(out, 0, sizeof(float) * samples);
	int samplesToCopy = samples;
	auto pvoutput = Planeverb::GetOutput(m_data.id);
	float dryGain = pvoutput.occlusion;

	if (!m_data.usePlaneverb)
	{
		dryGain = 1.f;
	}

	if (!m_data.currentlyPlaying || !m_data.dataPlaying)
	{
	}
	else if(!m_data.usePlaneverb)
	{
		float* dataArray = m_data.dataPlaying->data;
		float* outStart = out;
		float gain = m_data.volume;
		int readIndex = m_data.readIndex;
		dataArray += readIndex;
		int size = (int)m_data.dataPlaying->numSamples;
		int lastIndex = readIndex + samples;
		if (lastIndex >= size)
		{
			samplesToCopy = size - readIndex;
			m_data.currentlyPlaying = false;
			Planeverb::EndEmission(m_data.id);
		}

		for (int i = 0; i < samplesToCopy; ++i)
			*out++ += *dataArray++ * gain * dryGain;

		out = outStart;

		//AudioData temp;
		//temp.data = out;
		//temp.bitsPerSample = 16;
		//temp.numChannels = 2;
		//temp.numSamples = samplesToCopy;
		//temp.numFrames = temp.numSamples / 2;
		//temp.samplingRate = RATE;
		//temp.sizeInBytes = 2 * temp.numSamples;
		//AudioData::write_wave("weird_lowpass.wav", temp);
		//temp.data = nullptr;
		//__debugbreak();

		m_data.readIndex += samplesToCopy;
	}
	else
	{
		PlaneverbDSP::PlaneverbDSPInput dspInput;
		dspInput.lowpass = pvoutput.lowpass;
		dspInput.obstructionGain = pvoutput.occlusion;
		dspInput.wetGain = pvoutput.wetGain;
		dspInput.rt60 = pvoutput.rt60;
		dspInput.direction.x = pvoutput.direction.x;
		dspInput.direction.y = pvoutput.direction.y;
		dspInput.sourceDirectivity.x = pvoutput.sourceDirectivity.x;
		dspInput.sourceDirectivity.y = pvoutput.sourceDirectivity.y;

		float* dataArray = m_data.dataPlaying->data;
		float* outStart = out;
		float gain = m_data.volume;
		int readIndex = m_data.readIndex;
		dataArray += readIndex;
		int size = (int)m_data.dataPlaying->numSamples;
		int lastIndex = readIndex + samples;
		if (lastIndex >= size)
		{
			samplesToCopy = size - readIndex;
			m_data.currentlyPlaying = false;
			Planeverb::EndEmission(m_data.id);
		}
		
		//std::memcpy(out, dataArray, samplesToCopy * sizeof(float));
		PlaneverbDSP::SendSource(m_data.id, &dspInput, dataArray, samplesToCopy / CHANNELS);
		//std::memset(out, 0, sizeof(float) * samples);
		float* dry = nullptr;
		float* obA = nullptr;
		float* obB = nullptr;
		float* obC = nullptr;
		
		PlaneverbDSP::GetOutput(&dry, &obA, &obB, &obC);

		for (int i = 0; i < samplesToCopy; ++i)
		{
			// no reverb here
			*out++ = *dry++;
		}
		
		m_data.readIndex += samplesToCopy;
	}
}
