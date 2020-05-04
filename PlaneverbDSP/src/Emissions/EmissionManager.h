#pragma once
#include "PvDSPTypes.h"
#include "PvDSPDefinitions.h"
#include "DSP\Lowpass.h"
#include <unordered_map>

namespace PlaneverbDSP
{
	// data stored per emission
	struct EmissionData
	{
		LowpassFilter lpf[PV_DSP_CHANNEL_COUNT];	// lowpass filters for each channel
		float occlusion = 1.f;						// occlusion parameter
		float rt60 = 0.f;							// decay time parameter
		vec2 direction = { 0, 0 };					// direction parameter
		EmissionData(float samplingRate) :
			lpf{ {samplingRate}, {samplingRate} },
			occlusion(1.f),
			rt60(0.f),
			direction{ 0, 0 }
		{}
	};

	// Manages audio playback data at runtime
	class EmissionsManager
	{
		using EmissionMap = std::unordered_map<EmissionID, EmissionData>;

	public:
		EmissionsManager(float samplingRate) :
			m_emissionsCurrent(),
			m_emissionsTarget(),
			m_samplingRate(samplingRate)
		{}
		PV_DSP_INLINE EmissionData& GetDataCurrent(const EmissionID& id)
		{
			// finds id in the map, if doesn't exist makes a new one
			auto it = m_emissionsCurrent.find(id);
			if (it == m_emissionsCurrent.end())
			{
				auto r = m_emissionsCurrent.insert_or_assign(id, EmissionData(m_samplingRate));
				return r.first->second;
			}
			else
			{
				return it->second;
			}
		}

		PV_DSP_INLINE EmissionData& GetDataTarget(const EmissionID& id)
		{
			// finds in the target map, if doesn't exist, makes a new one
			auto it = m_emissionsTarget.find(id);
			if (it == m_emissionsTarget.end())
			{
				auto r = m_emissionsTarget.insert_or_assign(id, EmissionData(m_samplingRate));
				return r.first->second;
			}
			else
			{
				return it->second;
			}
		}

	private:
		EmissionMap m_emissionsCurrent;	// data for current emissions
		EmissionMap m_emissionsTarget;	// data for what emissions should target
		float m_samplingRate;			// audio engine sampling rate for LPF
	};
} // namespace PlaneverbDSP
