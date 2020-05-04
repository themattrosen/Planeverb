#pragma once

#include <PvTypes.h>	// vec2, vec3, Real

namespace Planeverb
{
	// Forward declares
	class Grid;
	class FreeGrid;
	struct Cell;

	// Internal structure used by analyzer, reflects the output parameters used by module
	struct AnalyzerResult
	{
		Real occlusion;
		Real rt60;
		Real lowpassIntensity;
		vec2 direction;
	};

	// Analyzes acoustic grid IR output
	class Analyzer
	{
	public:
		Analyzer(Grid* grid, FreeGrid* freeGrid);
		~Analyzer();

		void AnalyzeResponses(const vec3& listenerPos);
		const AnalyzerResult* GetResponseResult(const vec3& emitterPos) const;

	private:
		Real AnalyzeOcclusion(unsigned index, const Cell* response, const vec3& listenerPos, unsigned numSamples);
		Real AnalyzeLowpassIntensity(unsigned index, const Cell* response, const vec3& listenerPos, unsigned numSamples);
		Real AnalyzeDecayTime(unsigned index, const Cell* response, unsigned numSamples);
		Real AnalyzeDelay(unsigned index, const Cell* response, unsigned numSamples);
		vec2 AnalyzeDirection(unsigned index, const Cell* response, const vec3& listenerPos, unsigned numSamples);

		Grid* m_grid;				// handle to the grid system
		FreeGrid* m_freeGrid;		// handle to the free grid system
		unsigned m_gridX, m_gridY;	// number of cells in the grid x and y
		Real m_dx;					// meters per grid for conversions
		unsigned m_responseLength;	// number of samples per IR
		unsigned m_samplingRate;	// sampling rate for conversions (samples per second)
		unsigned m_numThreads;		// number of threads the module is allowed to use
		int m_resolution;			// grid resolution

		char* m_mem;				// pool of memory

		AnalyzerResult* m_results;	// 2D grid using 1D memory, grid of results
		Real* m_delaySamples;		// grid of delay, to be used to find direction
	};
} // namespace Planeverb
