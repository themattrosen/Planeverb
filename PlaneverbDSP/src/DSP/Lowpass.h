#pragma once
#include "PvDSPTypes.h"
#include "PvDSPDefinitions.h"
#include <cmath>

namespace PlaneverbDSP
{
	// 2nd-Order Butterworth LPF
	class LowpassFilter
	{
	public:
		LowpassFilter(float samplingRate, float cutoff = 20000.f);

		PV_DSP_INLINE void SetCutoff(float cutoffInHertz)
		{
			PV_DSP_ASSERT(cutoffInHertz <= PV_DSP_MAX_AUDIBLE_FREQ &&
				cutoffInHertz >= PV_DSP_MIN_AUDIBLE_FREQ);

			m_freqCutoff = cutoffInHertz;
			float cutoffInRad = 2.f * PV_DSP_PI * cutoffInHertz;
			float T = cutoffInRad / m_samplingRate;
			float Y = 1.f / (1.f + PV_DSP_SQRT_2 * T + T * T);
			m_xCoeff = T * T * Y;
			m_y1Coeff = (2.f + PV_DSP_SQRT_2 * T) * Y;
			m_y2Coeff = -1.f * Y;
		}
		PV_DSP_INLINE float GetCutoff() const { return m_freqCutoff; }
		
		// modifies buffer in place
		// channel is 0 or 1, assume stereo output
		// numFrames is the number of audio frames, not samples
		// lerps to a target cutoff with a given lerpfactor
		PV_DSP_INLINE void Process(float* bufferToModify, int channel, int maxChannels, int numFrames, float targetCutoff, float lerpFactor)
		{
			// make looping buffer
			float* buf = bufferToModify + channel;

			// find target values
			float targetCutoffRad = 2.f * PV_DSP_PI * targetCutoff;
			float targetT = targetCutoffRad / m_samplingRate;
			float targetY = 1.f / (1.f + PV_DSP_SQRT_2 * targetT + targetT * targetT);
			float targetX = targetT * targetT * targetY;
			float targetY1 = (2.f + PV_DSP_SQRT_2 * targetT) * targetY;
			float targetY2 = -1.f * targetY;

			// temporary current values
			float currentX = m_xCoeff;
			float currentY1 = m_y1Coeff;
			float currentY2 = m_y2Coeff;

			// process each input frame at the proper channel
			for (int frame = 0; frame < numFrames; ++frame)
			{
				// process filter function
				*buf = currentX * *buf +
					currentY1 * m_ydelay1 +
					currentY2 * m_ydelay2;

				// feed back delays
				m_ydelay2 = m_ydelay1;
				m_ydelay1 = *buf;

				// increment array ptr
				buf += maxChannels;

				// lerp multipliers
				currentX  = LERP_FLOAT(currentX,  targetX,  lerpFactor);
				currentY1 = LERP_FLOAT(currentY1, targetY1, lerpFactor);
				currentY2 = LERP_FLOAT(currentY2, targetY2, lerpFactor);
			}

			m_xCoeff  = currentX;
			m_y1Coeff = currentY1;
			m_y2Coeff = currentY2;
		}

	private:

		float m_freqCutoff;		// current cutoff frequency
		float m_samplingRate;	// audio engine sampling rate
		float m_ydelay1;		// output delay of 1 sample
		float m_ydelay2;		// output delay from 2 samples
		float m_xCoeff;			// input multiplier
		float m_y1Coeff;		// delay 1 multiplier
		float m_y2Coeff;		// delay 2 multiplier
	};
}