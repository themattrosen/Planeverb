#include "ImpulseResponse.h"
#include "PvDSPDefinitions.h"
#include <cmath>  // exp
#include <random> // random engine, distribution

namespace PlaneverbDSP
{
	ImpulseResponse::ImpulseResponse(float rt60, float samplingRate) : 
		m_inTimeDomain(nullptr),
		m_rt60(rt60),
		m_samplingRate(samplingRate)
	{
		m_arraySize = (unsigned)(m_samplingRate * m_rt60);

		m_inTimeDomain = new float[m_arraySize];
		std::default_random_engine generator;
		std::uniform_real_distribution<float> distribution(-1.f, 1.f);
		const constexpr float T60_CONSTANT = 6.91f;
		float tau = m_rt60 / T60_CONSTANT * m_samplingRate * -1.f;

		// generate enveloped noise
		for (int i = 0; i < (int)m_arraySize; ++i)
		{
			m_inTimeDomain[i] = distribution(generator) * std::exp((float)i / tau);
		}
	}

	ImpulseResponse::~ImpulseResponse()
	{
		PV_DSP_SAFE_ARRAY_DELETE(m_inTimeDomain);
	}

} // namespace PlaneverbDSP