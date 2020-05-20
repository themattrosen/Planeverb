#pragma once
#include <PvTypes.h>	// vec3
#include <thread>		// std::thread

namespace Planeverb
{
	// Forward declarations
	struct PlaneverbConfig;
	class Grid;
	class GeometryManager;
	class EmissionManager;
	class Analyzer;
	class FreeGrid;

	// Global context singleton that stores all systems
	class Context
	{
	public:
		// init/exit
		Context(const PlaneverbConfig* config);
		~Context();

		// getters
		const PlaneverbConfig* GetConfig() const { return &m_config; }
		Grid* GetGrid() { return m_grid; }
		FreeGrid* GetFreeGrid() { return m_freeGrid; }
		GeometryManager* GetGeometryManager() { return m_geometry; }
		Analyzer* GetAnalyzer() { return m_analyzer; }
		EmissionManager* GetEmissionManager() { return m_emissions; }
		bool IsRunning() const { return m_isRunning; }
		const vec3& GetListenerPosition() const { return m_listenerPos; }

		// setters
		void StopRunning() { m_isRunning = false; }
		void SetListenerPosition(const vec3& listenerPos) { m_listenerPos = listenerPos; }
		
	private:
		PlaneverbConfig m_config;			// copy of the input config
		std::thread m_backgroundProcessor;	// background thread handle
		bool m_isRunning = true;			// running flag used by thread

		vec3 m_listenerPos;					// global listener position

		char* m_systemMem;
		char* m_mem;						// all memory for systems stored linearly

		// FDTD manager
		Grid* m_grid;						// acoustic grid handle

		// geometry manager
		GeometryManager* m_geometry;		// geometry manager handle

		// emission manager
		EmissionManager* m_emissions;		// emission manager handle

		// response analyzer
		Analyzer* m_analyzer;				// analyzer handle
		
		// free grid
		FreeGrid* m_freeGrid;				// free grid handle
	};

	// Internal context singleton getter function
	Context* GetContext();
} // namespace Planeverb
