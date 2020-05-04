#include "PvDSPContext.h"
#include "DSP\Lowpass.h"
#include "Emissions\EmissionManager.h"

#include "DSP\ImpulseResponse.h"
#include "DSP\Convolver.h"

#include <cstring>
#include <cmath>

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
	void GetOutput(float** outA, float** outB, float** outC)
	{
		if (g_context)
			g_context->GetOutput(outA, outB, outC);
		else
		{
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
			m_bufferSize +					// 1 input temp storage buffer
			m_bufferSize * 3 * 2 +			// 3 ouput buffers, double buffered
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
		m_inputBufferStorage = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_outputBufferA_1 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_outputBufferB_1 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_outputBufferC_1 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_outputBufferA_2 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_outputBufferB_2 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_outputBufferC_2 = reinterpret_cast<float*>(temp); temp += m_bufferSize;
		m_outputBufferA = m_outputBufferA_1;
		m_outputBufferB = m_outputBufferB_1;
		m_outputBufferC = m_outputBufferC_1;

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

	// private functions to determine reverb lerp factors
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
		
		// determine each reverb gain target
		// A
		float revGainA = FindGainA(dspParams->rt60, dspParams->obstructionGain);

		// B
		float revGainB = FindGainB(dspParams->rt60, dspParams->obstructionGain);

		// C
		float revGainC = FindGainC(dspParams->rt60, dspParams->obstructionGain);

		// process source with lowpass
		auto& emissionData = m_emissions->GetDataTarget(id);
		emissionData.lpf[0].SetCutoff(dspParams->lowpass);
		emissionData.lpf[1].SetCutoff(dspParams->lowpass);
		emissionData.occlusion = dspParams->obstructionGain;
		emissionData.rt60 = dspParams->rt60;
		emissionData.direction.x = dspParams->direction.x;
		emissionData.direction.y = dspParams->direction.y;

		auto& currentData = m_emissions->GetDataCurrent(id);
		float currRevGainA = FindGainA(currentData.rt60, currentData.occlusion);
		float currRevGainB = FindGainB(currentData.rt60, currentData.occlusion);
		float currRevGainC = FindGainC(currentData.rt60, currentData.occlusion);

		// determine lerp factor
		float lerpFactor = 1.f / ((float)m_numFrames * (float)m_config.dspSmoothingFactor);

		// copy input into internal storage
		float* inputStoragePtr = m_inputBufferStorage;
		const float* inputPtr = in;
		for (int i = 0; i < numSamples; ++i)
		{
			*inputStoragePtr++ = *inputPtr++;
		}

		// process lowpass
		{
			auto* farr = emissionData.lpf;
			for (int i = 0; i < numChannels; ++i)
			{
				farr->Process(m_inputBufferStorage, i, numChannels, numFrames, dspParams->lowpass, lerpFactor);
				++farr;
			}
		}

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

		// add processed source with 
		// required gain to relevant inputBuffer
		{
			const int NUM_TASKS = 3;
			float* bufArray[NUM_TASKS] = {
				m_outputBufferA,
				m_outputBufferB,
				m_outputBufferC
			};
			float targetGainArray[NUM_TASKS] = { revGainA, revGainB, revGainC };
			float currentGainArray[NUM_TASKS] = { currRevGainA, currRevGainB, currRevGainC };
			auto processFunc = [&](float* buf, const float* in, float targetRevGain, float currRevGain, int frames)
			{
				for (int j = 0; j < frames; ++j)
				{
					*buf++ += *in++ * currRevGain * currentleft;
					*buf++ += *in++ * currRevGain * currentright;

					currRevGain = LERP_FLOAT(currRevGain, targetRevGain, lerpFactor);
					currentleft = LERP_FLOAT(currentleft, targetleft, lerpFactor);
					currentright = LERP_FLOAT(currentright, targetright, lerpFactor);
				}
			};

			for (int i = 0; i < NUM_TASKS; ++i)
			{
				processFunc(bufArray[i], m_inputBufferStorage, targetGainArray[i], currentGainArray[i], m_numFrames);
			}

			// lerp the real current data parameters
			for (int j = 0; j < m_numFrames; ++j)
			{
				currentData.direction.x = LERP_FLOAT(currentData.direction.x, emissionData.direction.x, lerpFactor);
				currentData.direction.y = LERP_FLOAT(currentData.direction.y, emissionData.direction.y, lerpFactor);
				currentData.occlusion = LERP_FLOAT(currentData.occlusion, emissionData.occlusion, lerpFactor);
				currentData.rt60 = LERP_FLOAT(currentData.rt60, emissionData.rt60, lerpFactor);
			}
		}

		currentData.lpf[0].SetCutoff(emissionData.lpf[0].GetCutoff());
		currentData.lpf[1].SetCutoff(emissionData.lpf[1].GetCutoff());
		
	}

	void Context::GetOutput(float** outA, float** outB, float** outC)
	{
		*outA = m_outputBufferA;
		*outB = m_outputBufferB;
		*outC = m_outputBufferC;

		// swap double buffers
		if (m_outputBufferA == m_outputBufferA_1)
		{
			m_outputBufferA = m_outputBufferA_2;
			m_outputBufferB = m_outputBufferB_2;
			m_outputBufferC = m_outputBufferC_2;
		}
		else
		{
			m_outputBufferA = m_outputBufferA_1;
			m_outputBufferB = m_outputBufferB_1;
			m_outputBufferC = m_outputBufferC_1;
		}
		
		// reset all memory in the fresh buffers
		std::memset(m_outputBufferA, 0, m_bufferSize * 3);
	}

	void Context::SetListenerTransform(const vec3 & position, const vec3 & forward)
	{
		m_listenerTransform.position = position;
		m_listenerTransform.forward = forward;
	}
}

