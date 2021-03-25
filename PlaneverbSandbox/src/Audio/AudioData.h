//Matthew Rosen
#pragma once

#include <fstream>
#include <cstring>

#define SHORT_TO_FLOAT(s) (static_cast<float>(s) / static_cast<float>((1 << (16 - 1))))
#define FLOAT_TO_SHORT(s) (static_cast<short>(s * static_cast<float>((1 << (16 - 1)) - 1)))

struct AudioData
{
	float samplingRate;
	unsigned sizeInBytes;
	unsigned numSamples;
	float * data;
	short bitsPerSample;
	short numChannels = 2;
	unsigned numFrames;

	AudioData() : samplingRate(0.f), sizeInBytes(0), numSamples(0), data(nullptr), bitsPerSample(0) {}

	AudioData(unsigned _numSamps, short _beets, float _sampR)
		: samplingRate(_sampR), sizeInBytes(0), numSamples(_numSamps),
		data(new float[_numSamps]), bitsPerSample(_beets)
	{
		sizeInBytes = static_cast<unsigned>(static_cast<float>(bitsPerSample) / 8.f * static_cast<float>(numSamples));
		std::memset(data, 0, sizeInBytes);
	}

	AudioData& operator=(const AudioData& rhs)
	{
		if (this != &rhs)
		{
			samplingRate = rhs.samplingRate;
			sizeInBytes = rhs.sizeInBytes;
			numSamples = rhs.numSamples;
			bitsPerSample = rhs.bitsPerSample;
			if (data)
				delete[] data;

			data = new float[numSamples];
			for (unsigned i = 0; i < numSamples; ++i)
				data[i] = rhs.data[i];
		}

		return *this;
	}

	~AudioData()
	{
		if (data)
		{
			delete[] data;
			data = nullptr;
		}
	}

	void normalize();

	static void read_wave(const char* filename, AudioData& adata);
	static void write_wave(const char* filename, const AudioData& data);

private:
	static void write_header(std::fstream& output, unsigned sizeInBytes);


};
