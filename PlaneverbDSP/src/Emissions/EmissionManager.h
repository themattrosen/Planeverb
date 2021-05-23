#pragma once
#include "PvDSPTypes.h"
#include "PvDSPDefinitions.h"
#include "DSP\Lowpass.h"
#include "PvDSPContext.h"
#include <vector>
#include <mutex>

namespace PlaneverbDSP
{
	// data stored per emission
	struct EmissionData
	{
		bool isValid;				 // validity of this data in the manager. when emitters are removed, this is set to false.
		LowpassFilter lpf;			 // lowpass filter for mono mixdown channel
		float occlusion = 1.f;		 // occlusion parameter
		float wetGain = 1.f;		 // wet gain for reverb
		float rt60 = 0.f;			 // decay time parameter
		vec2 direction = { 0, 0 };	 // direction parameter
		vec2 directivity = { 0, 0 }; // source directivity parameter
		vec3 position = { 0, 0, 0 };	 // position for distance attenuation
		vec3 forward = { 0, 0, 0 };	 // forward vector for spatialization
		vec3 up = { 0, 0, 0 };	// up vector for spatialization
		PlaneverbDSPSourceDirectivityPattern directivityPattern;
		EmissionData(float samplingRate, const vec3& pos, const vec3& _forward, const vec3& _up) :
			isValid(true),
			lpf{ samplingRate },
			occlusion(1.f),
			wetGain(1.f),
			rt60(0.f),
			direction{ 0, 0 },
			directivity{ 0, 0 },
			position{ pos },
			forward{ _forward },
			up{ _up },
			directivityPattern(pvd_Cardioid)
		{}
	};
#pragma optimize("", off)
	// Manages audio playback data at runtime
	class EmissionsManager
	{
		using EmissionMap = std::vector<EmissionData>;

	public:
		EmissionsManager(float samplingRate, unsigned maxEmitters) :
			m_numEmissions(0),
			m_maxEmissions(maxEmitters),
			m_emissionsCurrent(),
			m_emissionsTarget(),
			m_samplingRate(samplingRate),
			m_lastEmissionID(0),
			m_openSlots()
		{
			m_emissionsCurrent.reserve(maxEmitters);
			m_emissionsTarget.reserve(maxEmitters);
			m_openSlots.reserve(maxEmitters);
		}

		PV_DSP_INLINE EmissionID AddEmitter(const vec3& pos, const vec3& forward, const vec3& up)
		{
			EmissionID nextID = m_lastEmissionID;
			if (!m_openSlots.empty())
			{
				nextID = m_openSlots.back();
				m_openSlots.pop_back();
				m_emissionsCurrent[nextID] = EmissionData(m_samplingRate, pos, forward, up);
				m_emissionsTarget[nextID] = EmissionData(m_samplingRate, pos, forward, up);
			}
			else if(m_numEmissions + 1 <= m_maxEmissions)
			{
				++m_lastEmissionID;
				m_emissionsCurrent.emplace_back(EmissionData(m_samplingRate, pos, forward, up));
				m_emissionsTarget.emplace_back(EmissionData(m_samplingRate, pos, forward, up));
			}
			else // too many emitters, return error
			{
				PV_DSP_ASSERT(m_numEmissions + 1 <= m_maxEmissions && "Too many emitters created!");
				return PV_INVALID_EMISSION_ID;
			}

			++m_numEmissions;

			return nextID;
		}

		PV_DSP_INLINE EmissionData* GetDataCurrent(const EmissionID& id)
		{
			// finds id in the map
			if(id == PV_INVALID_EMISSION_ID || id >= m_emissionsCurrent.size())
				return nullptr;

			EmissionData* data(&m_emissionsCurrent[id]);
			if (!data->isValid)
				return nullptr;

			return data;
		}

		PV_DSP_INLINE EmissionData* GetDataTarget(const EmissionID& id)
		{
			// finds id in the map
			if (id == PV_INVALID_EMISSION_ID || id >= m_emissionsTarget.size())
				return nullptr;

			EmissionData* data(&m_emissionsTarget[id]);
			if (!data->isValid)
				return nullptr;

			return data;
		}

		PV_DSP_INLINE void RemoveEmission(const EmissionID& id)
		{
			if (id == PV_INVALID_EMISSION_ID || id >= m_emissionsTarget.size())
				return;

			m_emissionsCurrent[id].isValid = false;
			m_emissionsTarget[id].isValid = false;
			m_openSlots.push_back(id);
			--m_numEmissions;
		}

	private:
		unsigned m_numEmissions = 0;	// current number of emitters
		unsigned m_maxEmissions = 0;	// maximum number of emitters allowed
		EmissionMap m_emissionsCurrent;	// data for current emissions
		EmissionMap m_emissionsTarget;	// data for what emissions should target
		float m_samplingRate;			// audio engine sampling rate for LPF
		EmissionID m_lastEmissionID;	
		std::vector<EmissionID> m_openSlots;
	};
#pragma optimize("", on)
} // namespace PlaneverbDSP

