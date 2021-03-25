#include "PvDSPContext.h"
#include "DSP\Lowpass.h"
#include "Emissions\EmissionManager.h"

#include "DSP\ImpulseResponse.h"
#include "DSP\Convolver.h"

#include <cstring>
#include <cmath>
#include <algorithm>

namespace PlaneverbDSP
{
	#pragma region ClientInterface
	// Context singleton ptr
	static Context* g_context = nullptr;

	PV_DSP_INLINE Context* GetContext()
	{
		return g_context;
	}

	// allocates context ptr, exits if context already running
	void Init(const PlaneverbDSPConfig* config)
	{
		if (g_context)
		{
			Exit();
		}
		g_context = new Context(config);
	}

	// deallocates context ptr
	void Exit()
	{
		if (g_context)
		{
			delete g_context;
			g_context = nullptr;
		}
	}

	// sends source to the context
	void SendSource(EmissionID id, const PlaneverbDSPInput* dspParams,
		const float* in, unsigned numFrames)
	{
		if(g_context)
			g_context->SubmitSource(id, dspParams, in, numFrames);
	}

	// retrieves output from the context
	void GetOutput(float** dryOut, float** outA, float** outB, float** outC)
	{
		if (g_context)
			g_context->GetOutput(dryOut, outA, outB, outC);
		else
		{
			*dryOut = nullptr;
			*outA = nullptr;
			*outB = nullptr;
			*outC = nullptr;
		}
	}

	// sends listener data to the context
	void SetListenerTransform(float posX, float posY, float posZ,
		float forwardX, float forwardY, float forwardZ)
	{
		if(g_context)
			g_context->SetListenerTransform({ posX, posY, posZ }, { forwardX, forwardY, forwardZ });
	}

	void UpdateEmitter(EmissionID id, float posX, float posY, float posZ,
		float forwardX, float forwardY, float forwardZ)
	{
		if (g_context)
		{
			auto& data = g_context->GetEmissionManager()->GetDataTarget(id);
			data.forward = { forwardX, forwardZ };
			data.position = { posX, posZ };
		}
	}

	void SetEmitterDirectivityPattern(EmissionID id, PlaneverbDSPSourceDirectivityPattern pattern)
	{
		if (g_context)
			g_context->GetEmissionManager()->GetDataTarget(id).directivityPattern = pattern;
	}
	#pragma endregion

	Context::Context(const PlaneverbDSPConfig* config)
	{
		// copy the config
		std::memcpy(&m_config, config, sizeof(PlaneverbDSPConfig));

		// throw if input is invalid
		if (config->maxCallbackLength > PV_DSP_MAX_CALLBACK_LENGTH || config->dspSmoothingFactor <= 0)
		{
			throw pvd_InvalidConfig;
		}

		// find buffer size in bytes
		m_bufferSize = PV_DSP_CHANNEL_COUNT * config->maxCallbackLength * sizeof(float);

		// allocate memory all at once
		unsigned size =
			m_bufferSize / PV_DSP_CHANNEL_COUNT + // 1 input temp storage buffer, mono
			m_bufferSize * 4 * 2 +			// 4 ouput buffers, double buffered
			sizeof(EmissionsManager) +		// emissions manager
			sizeof(ImpulseResponse) +		// impulse response	- not currently supported
			sizeof(Convolver);				// convolver		- not currently supported
		m_mem = new char[size];
		if (!m_mem)
		{
			throw pvd_NotEnoughMemory;
		}
		std::memset(m_mem, 0, size);

		// place memory locations
		char* temp = m_mem;
		m_inputStorage = reinterpret_cast<float*>(temp); temp += m_bufferSize / PV_DSP_CHANNEL_COUNT;
		m_dryOutputBuffer_1 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_outputBufferA_1 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_outputBufferB_1 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_outputBufferC_1 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_dryOutputBuffer_2 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_outputBufferA_2 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_outputBufferB_2 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_outputBufferC_2 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_wetOutputA = m_outputBufferA_1;
		m_wetOutputB = m_outputBufferB_1;
		m_wetOutputC = m_outputBufferC_1;
		m_dryOutput = m_dryOutputBuffer_1;

		m_emissions = reinterpret_cast<EmissionsManager*>(temp); temp += sizeof(EmissionsManager);
		m_responseA = reinterpret_cast<ImpulseResponse*>(temp); temp += sizeof(ImpulseResponse);
		m_convolverA = reinterpret_cast<Convolver*>(temp); temp += sizeof(Convolver);

		m_emissions = new (m_emissions) EmissionsManager((float)m_config.samplingRate);
		m_responseA = new (m_responseA) ImpulseResponse(PV_DSP_T_ER_1, (float)m_config.samplingRate);
		m_convolverA = new (m_convolverA) Convolver(m_responseA);

		m_listenerTransform.position = { 0, 0, 0 };
		m_listenerTransform.forward  = { 1, 0, 0 };
	}

	Context::~Context()
	{
		m_convolverA->~Convolver();
		m_responseA->~ImpulseResponse();
		m_emissions->~EmissionsManager();

		// deallocate buffers
		PV_DSP_SAFE_ARRAY_DELETE(m_mem);
	}

	// private functions to determine reverb lerp factors and source directivity
	namespace
	{
		const constexpr float TSTAR = 0.1f;
		// we use gain = dryGain instead of 
		// gain = std::pow(10.f, -dryGain / 20.f);
		// because gain is stored as a linear gain factor instead of in dB

		PV_DSP_INLINE float FindGainA(float rt60, float dryGain)
		{
			if (rt60 > PV_DSP_T_ER_2)
			{
				return 0.f;
			}
			else if (rt60 < PV_DSP_T_ER_1)
			{
				return 1.f;
			}

			float gain = dryGain;
			float term1 = std::pow(10.f, -3.f * TSTAR / PV_DSP_T_ER_2);
			float term2 = std::pow(10.f, -3.f * TSTAR / rt60);
			float term3 = std::pow(10.f, -3.f * TSTAR / PV_DSP_T_ER_1);
			float a = gain * (term1 - term2) / (term1 - term3);
			return a;
		}

		PV_DSP_INLINE float FindGainB(float rt60, float dryGain)
		{
			if (rt60 < PV_DSP_T_ER_1)
			{
				return 0.f;
			}

			float gain = dryGain;
			float term2 = std::pow(10.f, -3.f * TSTAR / rt60);

			// case we want j + 1 instead of j
			if (rt60 > PV_DSP_T_ER_2)
			{
				float term1 = std::pow(10.f, -3.f * TSTAR / PV_DSP_T_ER_3);
				float term3 = std::pow(10.f, -3.f * TSTAR / PV_DSP_T_ER_2);
				float a = gain * (term1 - term2) / (term1 - term3);
				return a;
			}
			else
			{
				float term1 = std::pow(10.f, -3.f * TSTAR / PV_DSP_T_ER_2);
				float term3 = std::pow(10.f, -3.f * TSTAR / PV_DSP_T_ER_1);
				float a = gain * (term1 - term2) / (term1 - term3);
				return gain - a;
			}
		}

		PV_DSP_INLINE float FindGainC(float rt60, float dryGain)
		{
			if (rt60 > PV_DSP_T_ER_3)
			{
				return 1.f;
			}
			else if (rt60 < PV_DSP_T_ER_2)
			{
				return 0.f;
			}

			float gain = dryGain;
			float term1 = std::pow(10.f, -3.f * TSTAR / PV_DSP_T_ER_3);
			float term2 = std::pow(10.f, -3.f * TSTAR / rt60);
			float term3 = std::pow(10.f, -3.f * TSTAR / PV_DSP_T_ER_2);
			float a = gain * (term1 - term2) / (term1 - term3);
			return gain - a;
		}

		PV_DSP_INLINE float OmniPattern(const vec2& directivity, const vec2& forward)
		{
			return 1.f;
		}

		PV_DSP_INLINE float CardioidPattern(const vec2& directivity, const vec2& forward)
		{
			float dotValue = directivity.Dot(forward);
			float cardioid = (1.f + dotValue) / 2.f;
			return (cardioid > PV_DSP_MIN_DRY_GAIN) ? cardioid : PV_DSP_MIN_DRY_GAIN;
		}

		using SourceDirectivityPatternFunc = float(*)(const vec2& directivity, const vec2& forward);
		SourceDirectivityPatternFunc directivityPatternFuncs[pvd_SourceDirectivityPatternCount] = 
		{
			OmniPattern,
			CardioidPattern
		};
	} // namespace <>

	void Context::SubmitSource(EmissionID id, const PlaneverbDSPInput* dspParams,
		const float* in, unsigned numFrames)
	{
		m_numFrames = (int)numFrames > m_numFrames ? (int)numFrames : m_numFrames;
		int numChannels = (int)PV_DSP_CHANNEL_COUNT;
		int numSamples = numFrames * numChannels;

		// don't do anything if input is invalid
		if(dspParams->lowpass < PV_DSP_MIN_AUDIBLE_FREQ || dspParams->lowpass > PV_DSP_MAX_AUDIBLE_FREQ ||
			dspParams->obstructionGain <= 0.f ||
			(dspParams->direction.x == 0.f && dspParams->direction.y == 0.f))
		{
			return;
		}

		/////////////////////////////
		// Calculate all gains first

		// determine lerp factor
		float lerpFactor = 1.f / ((float)m_numFrames * (float)m_config.dspSmoothingFactor);

		// determine each reverb gain target
		float revGainA = FindGainA(dspParams->rt60, dspParams->wetGain);
		float revGainB = FindGainB(dspParams->rt60, dspParams->wetGain);
		float revGainC = FindGainC(dspParams->rt60, dspParams->wetGain);

		// get target and current emission data
		auto& emissionData = m_emissions->GetDataTarget(id);
		emissionData.lpf.SetCutoff(dspParams->lowpass);
		emissionData.occlusion = dspParams->obstructionGain;
		emissionData.wetGain = dspParams->wetGain;
		emissionData.rt60 = dspParams->rt60;
		emissionData.direction.x = dspParams->direction.x;
		emissionData.direction.y = dspParams->direction.y;
		emissionData.directivity.x = dspParams->sourceDirectivity.x;
		emissionData.directivity.y = dspParams->sourceDirectivity.y;

		auto& currentData = m_emissions->GetDataCurrent(id);
		float currRevGainA = FindGainA(currentData.rt60, currentData.wetGain);
		float currRevGainB = FindGainB(currentData.rt60, currentData.wetGain);
		float currRevGainC = FindGainC(currentData.rt60, currentData.wetGain);
		float currDryGain = currentData.occlusion;

		// determine panning current and target values
		float targetleft = 1.f, targetright = 1.f;
		float currentleft = 1.f, currentright = 1.f;
		if (m_config.useSpatialization)
		{
			vec2 dir = dspParams->direction;
			float angle = std::atan2f(m_listenerTransform.forward.z, m_listenerTransform.forward.x);
			float phi = std::atan2f(dir.y, dir.x);
			float aphi = std::abs(phi);
			float premapped = angle - phi;
			float theta = premapped / 2.f;
			float ct = std::cos(theta);
			float st = std::sin(theta);
			targetleft = PV_DSP_INV_SQRT_2 * (ct - st);
			targetright = PV_DSP_INV_SQRT_2 * (ct + st);

			dir = currentData.direction;
			phi = std::atan2f(dir.y, dir.x);
			aphi = std::abs(phi);
			premapped = angle - phi;
			theta = premapped / 2.f;
			ct = std::cos(theta);
			st = std::sin(theta);
			currentleft = PV_DSP_INV_SQRT_2 * (ct - st);
			currentright = PV_DSP_INV_SQRT_2 * (ct + st);
		}

		// figure out source directivity current and target values
		PlaneverbDSPSourceDirectivityPattern pattern = currentData.directivityPattern;
		float targetDirectivityGain = directivityPatternFuncs[pattern](emissionData.directivity, emissionData.forward);
		float currentDirectivityGain = directivityPatternFuncs[pattern](currentData.directivity, emissionData.forward);

		// figure out distance attenuation values
		//TODO: These should be 3D attenuation value
		vec2 dist = { m_listenerTransform.position.x - emissionData.position.x, m_listenerTransform.position.z - emissionData.position.y };
		float euclideanDistance = std::sqrt(dist.x * dist.x + dist.y * dist.y);
		euclideanDistance = (euclideanDistance < 1.f) ? 1.f : euclideanDistance;
		float targetDistanceAttenuation = 1.f / euclideanDistance;

		dist = { m_listenerTransform.position.x - currentData.position.x, m_listenerTransform.position.z - currentData.position.y };
		euclideanDistance = std::sqrt(dist.x * dist.x + dist.y * dist.y);
		euclideanDistance = (euclideanDistance < 1.f) ? 1.f : euclideanDistance;
		float currentDistanceAttenuation = 1.f / euclideanDistance;
		
		float targetDryGain = std::max(emissionData.occlusion, PV_DSP_MIN_DRY_GAIN);

		////////////////////////////////////////
		// Run all processing after calculation

		// copy input into internal storage -> Sum to mono
		float* inputStoragePtr = m_inputStorage;
		const float* inputPtr = in;
		for (unsigned i = 0; i < numFrames; ++i)
		{
			float left = *inputPtr++;
			float right = *inputPtr++;
			*inputStoragePtr++ = (left + right) * 0.5f;
		}
		inputStoragePtr = m_inputStorage;

		// process lowpass on copy of input signal
		emissionData.lpf.Process(m_inputStorage, 0, 1, numFrames, dspParams->lowpass, lerpFactor);

		// apply wet gain
		{
			const int NUM_TASKS = 3;
			float* bufArray[NUM_TASKS] = {
				m_wetOutputA,
				m_wetOutputB,
				m_wetOutputC
			};
			float targetGainArray[NUM_TASKS] = { revGainA, revGainB, revGainC };
			float currentGainArray[NUM_TASKS] = { currRevGainA, currRevGainB, currRevGainC };
			auto processFunc = [&](float* buf, const float* in, float targetRevGain, float currRevGain, int frames)
			{
				for (int j = 0; j < frames; ++j)
				{
					*buf++ = *in * currRevGain * m_config.wetGainRatio;
					*buf++ = *in * currRevGain * m_config.wetGainRatio;
					++in;
					currRevGain = LERP_FLOAT(currRevGain, targetRevGain, lerpFactor);
				}
			};

			for (int i = 0; i < NUM_TASKS; ++i)
			{
				processFunc(bufArray[i], inputStoragePtr, targetGainArray[i], currentGainArray[i], numFrames);
			}
		}

		// apply dry gains
		for (int i = 0; i < m_numFrames; ++i)
		{
			float nextGain = currDryGain * currentDirectivityGain * currentDistanceAttenuation;
			*inputStoragePtr++ *= nextGain;
			
			currDryGain = LERP_FLOAT(currDryGain, targetDryGain, lerpFactor);
			currentDirectivityGain = LERP_FLOAT(currentDirectivityGain, targetDirectivityGain, lerpFactor);
			currentDistanceAttenuation = LERP_FLOAT(currentDistanceAttenuation, targetDistanceAttenuation, lerpFactor);
		}
		inputStoragePtr = m_inputStorage;

		// apply spatialization
		float* dryPtr = m_dryOutput;
		inputStoragePtr = m_inputStorage;
		for (unsigned i = 0; i < numFrames; ++i)
		{
			*dryPtr++ += *inputStoragePtr * currentleft;
			*dryPtr++ += *inputStoragePtr * currentright;
			currentright = LERP_FLOAT(currentright, targetright, lerpFactor);
			currentleft = LERP_FLOAT(currentleft, targetleft, lerpFactor);
			++inputStoragePtr;
		}

		// lerp the real current data parameters
		// this can probably be vectorized at some point
		currentData.occlusion = currDryGain;
		for (int j = 0; j < m_numFrames; ++j)
		{
			currentData.direction.x = LERP_FLOAT(currentData.direction.x, emissionData.direction.x, lerpFactor);
			currentData.direction.y = LERP_FLOAT(currentData.direction.y, emissionData.direction.y, lerpFactor);
			currentData.wetGain = LERP_FLOAT(currentData.wetGain, emissionData.wetGain, lerpFactor);
			currentData.rt60 = LERP_FLOAT(currentData.rt60, emissionData.rt60, lerpFactor);
			currentData.forward.x = LERP_FLOAT(currentData.forward.x, emissionData.forward.x, lerpFactor);
			currentData.forward.y = LERP_FLOAT(currentData.forward.y, emissionData.forward.y, lerpFactor);
			currentData.directivity.x = LERP_FLOAT(currentData.directivity.x, emissionData.directivity.x, lerpFactor);
			currentData.directivity.y = LERP_FLOAT(currentData.directivity.y, emissionData.directivity.y, lerpFactor);
			currentData.position.x = LERP_FLOAT(currentData.position.x, emissionData.position.x, lerpFactor);
			currentData.position.y = LERP_FLOAT(currentData.position.y, emissionData.position.y, lerpFactor);
		}

		currentData.lpf.SetCutoff(emissionData.lpf.GetCutoff());
	}

	void Context::GetOutput(float** dryOut, float** outA, float** outB, float** outC)
	{
		*dryOut = m_dryOutput;
		*outA = m_wetOutputA;
		*outB = m_wetOutputB;
		*outC = m_wetOutputC;

		// swap double buffers
		if (m_wetOutputA == m_outputBufferA_1)
		{
			m_dryOutput = m_dryOutputBuffer_2;
			m_wetOutputA = m_outputBufferA_2;
			m_wetOutputB = m_outputBufferB_2;
			m_wetOutputC = m_outputBufferC_2;
		}
		else
		{
			m_dryOutput = m_dryOutputBuffer_1;
			m_wetOutputA = m_outputBufferA_1;
			m_wetOutputB = m_outputBufferB_1;
			m_wetOutputC = m_outputBufferC_1;
		}
		
		// reset all memory in the fresh buffers
		std::memset(m_dryOutput, 0, m_bufferSize * 4);
	}

	void Context::SetListenerTransform(const vec3 & position, const vec3 & forward)
	{
		m_listenerTransform.position = position;
		m_listenerTransform.forward = forward;
	}
}

