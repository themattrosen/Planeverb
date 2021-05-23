#include <Emissions\EmissionManager.h>
#include <Planeverb.h>
#include <Context\PvContext.h>

namespace Planeverb
{
#pragma region ClientInterface
	EmissionID AddEmitter(const vec3& emitterPosition)
	{
		auto* context = GetContext();
		if (context)
			return context->GetEmissionManager()->AddEmitter(emitterPosition);
		return PV_INVALID_EMISSION_ID;
	}

	void UpdateEmitter(EmissionID id, const vec3& position)
	{
		auto* context = GetContext();
		if (context)
			context->GetEmissionManager()->UpdateEmitter(id, position);
	}

	void RemoveEmitter(EmissionID id)
	{
		auto* context = GetContext();
		if (context)
			context->GetEmissionManager()->RemoveEmitter(id);
	}
#pragma endregion

	EmissionManager::~EmissionManager()
	{
		m_emitterPositions.~vector();
		m_openSlots.~vector();
	}

	EmissionID EmissionManager::AddEmitter(const vec3 & emitterPosition)
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

	void EmissionManager::UpdateEmitter(EmissionID id, const vec3 & pos)
	{
		int size = (int)m_emitterPositions.size();
		if(id >= 0 && id < size)
			m_emitterPositions[id] = pos;
	}

	void EmissionManager::RemoveEmitter(EmissionID id)
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
