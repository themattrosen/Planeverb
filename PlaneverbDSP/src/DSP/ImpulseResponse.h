#pragma once
#include "PvDSPDefinitions.h"

namespace PlaneverbDSP
{
	class ImpulseResponse
	{
	public:
		ImpulseResponse(float rt60, float samplingRate);
		~ImpulseResponse();

		PV_DSP_INLINE const float* GetTimeDomain() const { return m_inTimeDomain; }
		PV_DSP_INLINE unsigned GetArraySize() const { return m_arraySize; }
		PV_DSP_INLINE float GetRT60() const { return m_rt60; }

	private:
		float* m_inTimeDomain;	// buffer for enveloped noise
		unsigned m_arraySize;	// number of elements
		float m_rt60;			// decay time
		float m_samplingRate;	// sampling rate
	};

} // namespace PlaneverbDSP
