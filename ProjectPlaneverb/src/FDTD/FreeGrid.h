#pragma once
#include <PvTypes.h>
#include "Grid.h"

namespace Planeverb
{
	class FreeGrid
	{
	public:
		FreeGrid(const PlaneverbConfig* config, char* mem);
		~FreeGrid();

		Real GetEFreePerR(const vec2& listenerPos, vec2& emitterPos);
        Real GetEnergyAtOneMeter() const;
        static unsigned GetMemoryRequirement(const struct PlaneverbConfig* config);

	private:
		Real SimulateFreeFieldEnergy(const PlaneverbConfig* config);
		Real CalculateEFree(const Cell* response, int responseLength, Real samplingRate) const;

		Grid* m_grid;
		Real m_dx;
		Real m_EFree;
	};
} // namespace Planeverb
