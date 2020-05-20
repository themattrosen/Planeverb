#include <Emissions\EmissionManager.h>
#include <Planeverb.h>
#include <Context\PvContext.h>

namespace Planeverb
{
#pragma region ClientInterface
	EmissionID Emit(const vec3& emitterPosition)
	{
		auto* context = GetContext();
		if (context)
			return context->GetEmissionManager()->Emit(emitterPosition);
		return PV_INVALID_EMISSION_ID;
	}

	void UpdateEmission(EmissionID id, const vec3& position)
	{
		auto* context = GetContext();
		if (context)
			context->GetEmissionManager()->UpdateEmission(id, position);
	}

	void EndEmission(EmissionID id)
	{
		auto* context = GetContext();
		if (context)
			context->GetEmissionManager()->EndEmission(id);
	}
#pragma endregion

	EmissionManager::~EmissionManager()
	{
		m_emitterPositions.~vector();
		m_openSlots.~vector();
	}

	EmissionID EmissionManager::Emit(const vec3 & emitterPosition)
	{
		// case there is an ID that can be reused
		if (!m_openSlots.empty())
		{
			EmissionID next = m_openSlots.back();
			m_openSlots.pop_back();
			m_emitterPositions[next] = emitterPosition;
			return next;
		}
		// case a new ID needs to be generated
		else
		{
			EmissionID next = m_emitterPositions.size();
			m_emitterPositions.push_back(emitterPosition);
			return next;
		}
	}

	void EmissionManager::UpdateEmission(EmissionID id, const vec3 & pos)
	{
		int size = (int)m_emitterPositions.size();
		if(id >= 0 && id < size)
			m_emitterPositions[id] = pos;
	}

	void EmissionManager::EndEmission(EmissionID id)
	{
		// add to the open slots to be reused
		m_openSlots.push_back(id);
	}

	const vec3* EmissionManager::GetEmitter(EmissionID id) const
	{
		int size = (int)m_emitterPositions.size();
		if(id >= 0 && id < size)
			return &m_emitterPositions[id];
		return nullptr;
	}

	unsigned EmissionManager::GetMemoryRequirement(const PlaneverbConfig * config)
	{
		return 0;
	}

}
