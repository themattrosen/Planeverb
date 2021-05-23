#pragma once

#include <PvTypes.h>
#include <vector>

namespace Planeverb
{
	// Keeps track of playing sounds, and distributes emitter IDs
	class EmissionManager
	{
	public:
		EmissionManager(char* mem) :
			m_emitterPositions(),
			m_openSlots()
		{
		}

		~EmissionManager();

		EmissionID AddEmitter(const vec3& emitterPosition);
		void UpdateEmitter(EmissionID id, const vec3& pos);
		void RemoveEmitter(EmissionID id);

		const vec3* GetEmitter(EmissionID id) const;
		static unsigned GetMemoryRequirement(const struct PlaneverbConfig* config);
	private:
		std::vector<vec3> m_emitterPositions;	// dynamic array of current emitter positions, ID is index into vector
		std::vector<EmissionID> m_openSlots;	// dynamic array of open slots into the emitter positions vector, handles dynamic sources
	};
} // namespace Planeverb
