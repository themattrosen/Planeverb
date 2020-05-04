#pragma once
#include "ImpulseResponse.h"
#include "PvDSPTypes.h"
#include "PvDSPDefinitions.h"
#include <cstring> // memset
#include <cmath>

namespace PlaneverbDSP
{
	// unused convolver
	class Convolver
	{
	public:
		Convolver(const ImpulseResponse* response) :
			m_response(response)
		{
			std::memset(m_outputBuffer1, 0, sizeof(float) * PV_DSP_MAX_CALLBACK_LENGTH);
			std::memset(m_outputBuffer2, 0, sizeof(float) * PV_DSP_MAX_CALLBACK_LENGTH);
			m_outputBuffer = m_outputBuffer1;
			m_target = std::pow(10.0f, -1.5f / 20.0f);

		}

		~Convolver()
		{
			std::memset(m_outputBuffer1, 0, sizeof(float) * PV_DSP_MAX_CALLBACK_LENGTH);
			std::memset(m_outputBuffer2, 0, sizeof(float) * PV_DSP_MAX_CALLBACK_LENGTH);
		}

		PV_DSP_INLINE float* operator()(float* input, int numFrames)
		{
			return nullptr;
		}
	private:
		const ImpulseResponse* m_response;
		float m_outputBuffer1[PV_DSP_MAX_CALLBACK_LENGTH];
		float m_outputBuffer2[PV_DSP_MAX_CALLBACK_LENGTH];
		float* m_outputBuffer;
		float m_target;
	};
} // namespace PlaneverbDSP
