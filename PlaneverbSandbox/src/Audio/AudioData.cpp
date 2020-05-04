//Matthew Rosen

#include "AudioData.h"
#include "Util.h"
#include <PvDefinitions.h>
#include <iostream>
#include <cmath>

void AudioData::normalize()
{
	AudioData& data = *this;
	float maxVal = 0.f;
	const float dB = -1.5f;
	for (unsigned i = 0; i < data.numSamples; ++i)
	{
		if (std::abs(data.data[i]) > maxVal)
			maxVal = std::abs(data.data[i]);
	}

	float target = std::pow(10.0f, dB / 20.0f);
	float gain = target / maxVal;

	for (unsigned i = 0; i < data.numSamples; ++i)
	{
		data.data[i] *= gain;
	}
}

void AudioData::read_wave(const char* filename, AudioData& adata)
{
	std::ifstream file(filename, std::ios_base::in | std::ios_base::binary);
	if (!file)
	{
		std::cout << "File [" << filename << "] couldn't be opened!" << std::endl;
		return;
	}

	struct { char label[4]; unsigned size; } chunk_head;
	char h[4];
	file.read(reinterpret_cast<char *>(&chunk_head), 8);
	file.read(h, 4);
	if (!file || strncmp(chunk_head.label, "RIFF", 4) != 0
		|| strncmp(h, "WAVE", 4) != 0)
	{
		file.close();
		std::cout << "File [" << filename << "] didn't have RIFF and WAVE specifier!" << std::endl;
		return;
	}

	///////////////////////
	// find the fmt  label
	file.read(reinterpret_cast<char *>(&chunk_head), 8);
	while (file && strncmp(chunk_head.label, "fmt ", 4) != 0)
	{
		file.seekg(chunk_head.size, std::ios_base::cur);
		file.read(reinterpret_cast<char *>(&chunk_head), 8);
	}
	if (!file)
	{
		file.close();
		std::cout << "File [" << filename << "] didn't have fmt specifier!" << std::endl;
		return;
	}

	char fmt[16] = { 0 };
	file.read(fmt, 16);

	///////////////////////////////////
	// store format struct information
	using u16 = unsigned short;
	using u32 = unsigned;
	using f32 = float;
	using s16 = short;
	struct Format
	{
		u16 audio_format;
		u16 channel_count;
		u32 sampling_rate;
		u32 bytes_per_second;
		u16 bytes_per_sample;
		u16 bits_per_sample;
	} format = { 0 };
	format.audio_format = *reinterpret_cast<unsigned short *>(fmt + 0);

	format.channel_count = *reinterpret_cast<unsigned short *>(fmt + 2);

	format.sampling_rate = *reinterpret_cast<unsigned int   *>(fmt + 4);

	format.bytes_per_second = *reinterpret_cast<unsigned int   *>(fmt + 8);

	format.bytes_per_sample = *reinterpret_cast<unsigned short *>(fmt + 12);

	format.bits_per_sample = *reinterpret_cast<unsigned short *>(fmt + 14);

	// make sure audio format is uncompressed
	if (format.audio_format != 1)
	{
		file.close();
		std::cout << "File [" << filename << "] format isn't uncompressed in .wav file" << std::endl;
		return;
	}

	///////////////////////
	// find the data label
	file.read(reinterpret_cast<char *>(&chunk_head), 8);
	while (file && strncmp(chunk_head.label, "data", 4) != 0)
	{
		file.seekg(chunk_head.size, std::ios_base::cur);
		file.read(reinterpret_cast<char *>(&chunk_head), 8);
	}
	// check if went to the end without finding
	if (!file)
	{
		file.close();
		throw std::runtime_error("Couldn't find data label in .wav file");
	}

	///////////////////////////////////////////
	// retrieve information from the data size 
	u32 size = chunk_head.size;

	if (size == 0)
	{
		adata.data = nullptr;
		std::cout << "File [" << filename << "] had no data!" << std::endl;
		return;
	}

	adata.bitsPerSample = format.bits_per_sample;
	adata.samplingRate = (float)format.sampling_rate;
	adata.numChannels = format.channel_count;
	adata.sizeInBytes = size;
	adata.numSamples = size / format.bytes_per_sample;
	adata.numFrames = adata.numSamples / format.channel_count;

	if (!(adata.bitsPerSample == 16 || adata.bitsPerSample == 8 || adata.bitsPerSample == 24))
	{
		adata.data = nullptr;
		std::cout << "File [" << filename << "] has an unsupported bit rate (" << adata.bitsPerSample << ")!" << std::endl;
		return;
	}

	char* data = new char[size];
	PV_ASSERT(data != nullptr && "New failed!");
	file.read(reinterpret_cast<char *>(data), size);

	if (!(adata.numChannels == 1 || adata.numChannels == 2))
	{
		delete[] data;
		std::cout << "File [" << filename << "] has unsupported number of channels (" << adata.numChannels << ")!" << std::endl;
		return;
	}

	adata.data = new float[adata.numSamples];
	PV_ASSERT(adata.data != nullptr && "New failed!");
	const f32 denom = static_cast<f32>((1 << (adata.bitsPerSample - 1)));

	switch (adata.bitsPerSample)
	{
	case 8:
	{
		for (u32 i = 0; i < adata.numSamples; ++i)
		{
			f32 nextVal = static_cast<f32>(*(data + i)) / denom;
			adata.data[i] = nextVal - 1.0f;
		}
		break;
	}
	case 16:
	{
		for (u32 i = 0; i < adata.numSamples; ++i)
		{
			s16 val = *reinterpret_cast<s16*>(data + i * sizeof(s16));
			f32 nextVal = static_cast<f32>(val) / denom;
			adata.data[i] = nextVal;
		}
		break;
	}
	case 24:
	{
		for (u32 i = 0; i < adata.numSamples; ++i)
		{
			//TODO
		}
		break;
	}
	}

	delete[] data;
}

// write audio data out to a wave file
void AudioData::write_wave(const char* filename, const AudioData& data)
{
	std::fstream out(filename, std::ios_base::binary | std::ios_base::out);

	short* outData = new short[data.numSamples];

	for (unsigned i = 0; i < data.numSamples; ++i)
	{
		float nextF = data.data[i];
		short nextS = FLOAT_TO_SHORT(nextF);
		outData[i] = nextS;
	}

	write_header(out, data.sizeInBytes);
	out.write(reinterpret_cast<char*>(outData), data.sizeInBytes);
	out.close();
	delete[] outData;
}


// write a wav header to a file
void AudioData::write_header(std::fstream& output, unsigned sizeInBytes)
{
	struct {
		char riff_chunk[4];
		unsigned chunk_size;
		char wave_fmt[4];
		char fmt_chunk[4];
		unsigned fmt_chunk_size;
		unsigned short audio_format;
		unsigned short number_of_channels;
		unsigned sampling_rate;
		unsigned bytes_per_second;
		unsigned short block_align;
		unsigned short bits_per_sample;
		char data_chunk[4];
		unsigned data_chunk_size;
	}
	header = { {'R','I','F','F'},
			   36 + sizeInBytes,
			   {'W','A','V','E'},
			   {'f','m','t',' '},
			   16,1,2,RATE,sizeof(short) * RATE,2,16,
			   {'d','a','t','a'},
			   sizeInBytes
	};

	output.write(reinterpret_cast<char*>(&header), 44);
}
