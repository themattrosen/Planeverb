#include "DSP\Lowpass.h"
#include "PvDSPTypes.h"
#include "PvDSPDefinitions.h"

namespace PlaneverbDSP
{
	LowpassFilter::LowpassFilter(float samplingRate, float cutoff) :
		m_freqCutoff(cutoff),
		m_samplingRate(samplingRate),
		m_ydelay1(0.f),
		m_ydelay2(0.f)
	{
		SetCutoff(m_freqCutoff);
	}
}
