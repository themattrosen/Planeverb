#pragma once
#include "PvTypes.h"
#include <vector>

namespace Planeverb
{
	// struct to represent wall information
	// 12 bytes
	struct BoundaryInfo
	{
		vec2 normal;		// wall normal, (0, 0) if no wall
		Real absorption;	// absorption coefficient, 0 if free space
		BoundaryInfo(const vec2& n = vec2(0, 0), Real a = Real(1.0)) :
			normal(n),
			absorption(a)
		{}

		BoundaryInfo& operator=(const BoundaryInfo&) = default;
	};

	// struct to represent grid cells
	// 16 bytes
	struct Cell
	{
		Real pr;	// air pressure
		Real vx;	// x component of particle velocity
		Real vy;	// y component of particle velocity
		short b;	// B field packed into 2 2 byte fields
		short by;	// B field packed into 2 2 byte fields

		Cell(Real _pr = 0.f, Real _vx = 0.f, Real _vy = 0.f, int boundaryCoef = 1, int _by = 1) :
			pr(_pr),
			vx(_vx),
			vy(_vy),
			b((short)boundaryCoef),
			by((short)_by)
		{}
	};

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
		Real GetDX() const { return m_dx; }
		int GetResolution() const { return m_resolution; }

		void AddAABB(const AABB* transform);
		void RemoveAABB(const AABB* transform);
		void UpdateAABB(const AABB* oldTransform, const AABB* newTransform);

		void PrintGrid();
		static unsigned GetMemoryRequirement(const struct PlaneverbConfig* config);
	private:
		char* m_mem;								// memory pool
		Cell* m_grid;								// cell grid
		BoundaryInfo* m_boundaries;					// wall information

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
		PlaneverbExecutionType m_executionType;		// use CPU or GPU (only CPU implemented so far
		unsigned m_maxThreads;						// thread usage
		int m_resolution;							// grid resolution
		Real m_z_inv;								// air absorption parameter
	};
} // namespace Planeverb
