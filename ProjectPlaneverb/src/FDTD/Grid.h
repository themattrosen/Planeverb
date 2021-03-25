#pragma once
#include "PvTypes.h"
#include <Util/Array.h>
#include <vector>
#include <mutex>

namespace Planeverb
{
	void CalculateGridParameters(int resolution, Real& dx, Real& dt, Real& samplingRate);

	// Grid system
	class Grid
	{
	public:
		// system init/exit
		Grid(const PlaneverbConfig* config, char* mem);
		~Grid();

		void GenerateResponseCPU(const vec3& listener);
		void GenerateResponseGPU(const vec3& listener);
		void GenerateResponse(const vec3& listener);
		Cell* GetResponse(const vec2& gridPosition);

		void AddAABB(const AABB* transform);
		void RemoveAABB(const AABB* transform);
		void ClearAABBs();
		void WorldToGrid(const vec2& worldspace, int& gridx, int& gridy) const;
		void GridToWorld(int gridx, int gridy, vec2& worldspace) const;

		void PrintGrid();
		static unsigned GetMemoryRequirement(const struct PlaneverbConfig* config);
	private:
		char* m_mem;								// memory pool
		Cell* m_grid;								// cell grid

		// originally used a 3D array of Cells for pulse response, 
		// but each access to it was probably a cache miss because of the length
		// of each response anyway, so 
		// it has been converted to being a 2D array of std::vectors
		Planeverb::Array<Cell>* m_pulseResponse;

		Real* m_pulse;								// precomputed Gaussian pulse
	};
} // namespace Planeverb
