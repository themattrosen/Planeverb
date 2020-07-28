#include <queue>
#include <cmath>

const constexpr int NUM_COMBS = 4;
const constexpr int COMB_ATTEN = 2; //std::sqrt(NUM_COMBS);
const constexpr float COMB_GAIN_BASE = 0.001f;
const constexpr int DEFAULT_MOVING_AVERAGE_WINDOW_SIZE = 10;
const constexpr int NUM_ALLPASS = 2;
const constexpr float LOWPASS_GAIN = 0.5f;
const constexpr int MAX_BUFFER_SIZE = 4096;
const constexpr int NUM_BUFFERS = NUM_COMBS;
const constexpr float COMB_DELAYS[NUM_COMBS] = 
{
	29.7f, 37.1f, 41.1f, 43.7f
};

const constexpr float ALLPASS_DELAYS[NUM_ALLPASS] = 
{
	5.0f, 1.7f
};

const constexpr float ALLPASS_RVT[NUM_ALLPASS] = 
{
	96.83f, 32.92f
};

class Reverb
{
	using DelayBuffer = std::queue<float>;

public:
	Reverb(float decayTime, float wetRatio, float samplingRate, int movingAverageWindowSize = DEFAULT_MOVING_AVERAGE_WINDOW_SIZE) : 
		m_decayTime{decayTime},
		m_wetRatio{wetRatio},
		m_samplingRate{samplingRate},
		m_combFilters
		{
			{ std::pow(COMB_GAIN_BASE, COMB_DELAYS[0] / decayTime), COMB_DELAYS[0], LOWPASS_GAIN, samplingRate},
			{ std::pow(COMB_GAIN_BASE, COMB_DELAYS[1] / decayTime), COMB_DELAYS[1], LOWPASS_GAIN, samplingRate},
			{ std::pow(COMB_GAIN_BASE, COMB_DELAYS[2] / decayTime), COMB_DELAYS[2], LOWPASS_GAIN, samplingRate},
			{ std::pow(COMB_GAIN_BASE, COMB_DELAYS[3] / decayTime), COMB_DELAYS[3], LOWPASS_GAIN, samplingRate},
		},
		m_allpassFilters
		{
			{ samplingRate, std::pow(COMB_GAIN_BASE, ALLPASS_DELAYS[0] / ALLPASS_RVT[0]), ALLPASS_DELAYS[0]},
			{ samplingRate, std::pow(COMB_GAIN_BASE, ALLPASS_DELAYS[1] / ALLPASS_RVT[1]), ALLPASS_DELAYS[1]}
		},
		m_noiseFilter{ movingAverageWindowSize }
	{}
	~Reverb() = default;
	
	inline void Process(const float* in, float* out, int channel, int maxChannels, int numFrames)
	{
		const float* inStart = in;
		float* outStart = out;
		const float* drySignal = in;
		int bufferIndex = 0; // current output buffer in use
		
		// split signal for combs in parallel
		for(int j = 0; j < NUM_COMBS; ++j)
		{
			m_combFilters[j].Process(in, m_buffers[j], channel, maxChannels, numFrames);
		}
		
		// accumulate wet signal into first buffer
		float* accum = m_bufferMem + channel;
		bufferIndex = 0;
		for(int j = 1; j < NUM_COMBS; ++j)
		{
			float* nextBuffer = m_buffers[j];
			for(int i = 0; i < numFrames; ++i, accum += maxChannels, nextBuffer += maxChannels)
			{
				*accum += *nextBuffer;
			}
			
			// reset start of accum buffer
			accum = m_bufferMem + channel;
		}
		
		// attenuate wet signal by comb attenuation
		for(int i = 0; i < numFrames; ++i, accum += maxChannels)
		{
			*accum /= COMB_ATTEN;
		}
		
		// feed wet signal into allpass filters
		float* allpassInput = m_bufferMem;
		float* allpassOutput;
		for(int j = 0; j < NUM_ALLPASS; ++j)
		{
			bufferIndex = (bufferIndex + 1) % NUM_BUFFERS;
			allpassOutput = m_buffers[bufferIndex];
			
			m_allpassFilters[j].Process(allpassInput, allpassOutput, channel, maxChannels, numFrames);
			
			allpassInput = allpassOutput;
		}
		
		// join wet signal and dry signal
		float* wetSignal = m_buffers[bufferIndex] + channel; // wet signal is output from allpass
		bufferIndex = (bufferIndex + 1) % NUM_BUFFERS; // increment output buffer index
		float* noisySignal = m_buffers[bufferIndex] + channel; // output of joining the wet and dry
		in = inStart + channel;	// set start of dry signal
		for(int i = 0; i < numFrames; ++i, in += maxChannels, noisySignal += maxChannels, wetSignal += maxChannels)
		{
			*noisySignal = *wetSignal * m_wetRatio + *in;
		}
		
		// filter noise
		noisySignal = m_buffers[bufferIndex]; // reset noisySignal ptr
		m_noiseFilter.Process(noisySignal, out, channel, maxChannels, numFrames);
	}
	
private:
	// filter information
	float m_decayTime;
	float m_wetRatio;
	float m_samplingRate;
	
	// TODO: early reflections
	
	// late reflections
	CombFilter m_combFilters[NUM_COMBS];
	float m_bufferMem[NUM_COMBS * MAX_BUFFER_SIZE];
	float* m_buffers[NUM_COMBS] = 
	{
		m_bufferMem + MAX_BUFFER_SIZE * 0,
		m_bufferMem + MAX_BUFFER_SIZE * 1,
		m_bufferMem + MAX_BUFFER_SIZE * 2,
		m_bufferMem + MAX_BUFFER_SIZE * 3
	};
	AllpassFilter m_allpassFilters[NUM_ALLPASS];
	
	// noise reduction, signal smoothing
	MovingAverageFilter m_noiseFilter;
	
	static int CalculateNumDelays(float samplingRate, float msDelay)
	{
		return static_cast<unsigned>(msDelay / 1000.f * static_cast<float>(samplingRate));
	}
		
	
	struct AllpassFilter
	{
		DelayBuffer xDelays;
		DelayBuffer yDelays;
		float gain;
		float msDelay;
		float samplingRate
		
		AllPassFilter(float samplingRate_, float gain_, float msDelay_) :
			 xDelays{}, yDelays{}, gain{gain_}, msDelay{msDelay_}, samplingRate{samplingRate_}
		{
			int numDelays = Reverb::CalculateNumDelays(samplingRate, msDelay);
			for (int i = 0; i < numDelays; ++i)
			{
				xdelays.push(0);
				ydelays.push(0);
			}
		}
		~AllPassFilter() = default;
		
		inline void Process(const float* in, float* out, int channel, int maxChannels, int numFrames)
		{
			in += channel;
			out += channel;
			for(int i = 0; i < numFrames; ++i, in += maxChannels, out += maxChannels)
			{
				float xDelay = xDelays.front();
				float yDelay = yDelays.front();
				xDelays.pop();
				yDelays.pop();
				
				*out = gain * *in + xDelay - gain * yDelay;
				xDelays.push(*in);
				yDelays.push(*out);
			}
		}
	};
	
	struct CombFilter
	{
		float combGain;
		float msDelay;
		float samplingRate;
		LowpassFilter lowpassFilter;
		DelayBuffer outputDelays;
		
		CombFilter(float combGain_, float msDelay_, float lowpassGain, float samplingRate_) :
			combGain{combGain_}, msDelay{msDelay_}, samplingRate{samplingRate_},
			lowpassFilter{lowpassGain}, outputDelays{}
		{
			int numDelays = Reverb::CalculateNumDelays(samplingRate, msDelay);
			for (int i = 0; i < numDelays; ++i)
			{
				outputDelays.push(0.f);
			}
		}
		~CombFilter() = default;
		
		inline void Process(const float* in, float* out, int channel, int maxChannels, int numFrames)
		{
			in += channel;
			out += channel;
			for(int i = 0; i < numFrames; ++i, in += maxChannels, out += maxChannels)
			{
				float x = outputDelays.front();
				outputDelays.pop();
				float output = *in + lowpassFilter(x) * combGain;
				outputDelays.push(output);
				*out = output;
			}
		}
	};
	
	struct MovingAverageFilter
	{
		float windowSize;
		float movingSum;
		DelayBuffer previousSamples;
		
		MovingAverageFilter(int windowSize_) :
			windowSize{static_cast<float>(windowSize_)}, movingSum{0.f}, previousSamples{}
		{
			for(int i = 0; i < windowSize_; ++i)
			{
				previousSamples.push(0.f);
			}
		}
		~MovingAverageFilter() = default;
		
		inline void Process(const float* in, float* out, int channel, int maxChannels, int numFrames)
		{
			in += channel;
			out += channel;
			
			for(int i = 0; i < numFrames; ++i, in += maxChannels, out += maxChannels)
			{
				movingSum += *in;
				movingSum -= previousSamples.front();
				previousSamples.pop();
				previousSamples.push(*in);
				*out = movingSum / windowSize;
			}
		}
	};
	
	struct LowpassFilter
	{
		float gain;
		float yDelay;
		
		explicit LowpassFilter(float g) : gain{g}, yDelay{0.f} {}
		~LowpassFilter() = default;
		
		inline void Process(const float* in, float* out, int channel, int maxChannels, int numFrames)
		{
			out += channel;
			in += channel;
			
			for(int i = 0; i < numFrames; ++i, out += maxChannels, in += maxChannels)
			{
				yDelay = *in + gain * yDelay;
				*out = yDelay;
			}
		}
		
		inline float operator()(float in)
		{
			yDelay = in + gain * yDelay;
			return yDelay;
		}
	};
	
};