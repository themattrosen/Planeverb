#pragma once
#include "PvTypes.h"
#include <vector>
#include <mutex>

namespace Planeverb
{
	void CalculateGridParameters(int resolution, Real& dx, Real& dt, unsigned& samplingRate);

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
		unsigned GetResponseSize() const;

		unsigned GetSamplingRate() const { return m_samplingRate; }
		unsigned GetMaxThreads() const { return m_maxThreads; }
		const vec2& GetGridSize() const { return m_gridSize; }
		const vec2& GetGridOffset() const { return m_gridOffset; }
		PlaneverbGridCenteringType GetCenteringType() const { return m_centering; }
		Real GetDX() const { return m_dx; }
		int GetResolution() const { return m_resolution; }

		void AddAABB(const AABB* transform);
		void RemoveAABB(const AABB* transform);
		void ClearAABBs();
		void WorldToGrid(const vec2& worldspace, int& gridx, int& gridy) const;

		void PrintGrid();
		static unsigned GetMemoryRequirement(const struct PlaneverbConfig* config);
	private:
		char* m_mem;								// memory pool
		Cell* m_grid;								// cell grid

		// originally used a 3D array of Cells for pulse response, 
		// but each access to it was probably a cache miss because of the length
		// of each response anyway, so 
		// it has been converted to being a 2D array of std::vectors
		std::vector<Cell>* m_pulseResponse;

		Real* m_pulse;								// precomputed Gaussian pulse

		Real m_dx;									// meters per grid cell
		Real m_dt;									// seconds per sample
		vec2 m_gridSize;							// grid size (in cells)
		vec2 m_gridDimensions;						// grid size (in meters)
		vec2 m_gridOffset;							// our grid uses only first quadrant, user uses all four, not currently implemented fully
		unsigned m_responseLength;					// number of samples for an IR
		unsigned m_samplingRate;					// samples per second
		PlaneverbExecutionType m_executionType;		// use CPU or GPU (only CPU implemented so far)
		unsigned m_maxThreads;						// thread usage
		int m_resolution;							// grid resolution
		PlaneverbGridCenteringType m_centering;		// grid centering type
	};
} // namespace Planeverb
