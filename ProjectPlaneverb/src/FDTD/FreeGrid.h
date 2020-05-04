#pragma once
#include <PvTypes.h>
#include "Grid.h"	// Cell

#include <string>	// string
#include <fstream>	// ifstream

namespace Planeverb
{
	class FreeGrid
	{
	public:
		FreeGrid(const PlaneverbConfig* config);
		~FreeGrid();

		Real GetEFreePerR(int listenerIndX, int listenerIndY, int emitterIndX, int emitterIndY);


	private:

		void ParseFile(const PlaneverbConfig* config, std::ifstream& file);
		void GenerateFile(const PlaneverbConfig* config, const std::string& fileName);

		Real CalculateEFree(const Cell* response, int responseLength,
			int lX, int lY, int samplingRate) const;

		Grid* m_grid;
		Real m_dx;
		Real m_EFree;
	};
} // namespace Planeverb
