#include <Context\PvContext.h>
#include <PvTypes.h>
#include <FDTD\Grid.h>
#include <Geometry\GeometryManager.h>
#include <Emissions\EmissionManager.h>
#include <DSP\Analyzer.h>
#include <FDTD\FreeGrid.h>
#include <Util\ScopedTimer.h>
#include <Planeverb.h>

#include <cstring>

namespace Planeverb
{
	// Context singleton ptr
	static Context* g_context = nullptr;

	#pragma region ClientInterface
	Context* GetContext()
	{
		return g_context;
	}

	// allocate context ptr. exits if context already running
	void Init(const PlaneverbConfig* config)
	{
		if (g_context)
		{
			Exit();
		}
		g_context = new Context(config);
	}

	// deallocates context ptr
	void Exit()
	{
		if (g_context)
		{
			delete g_context;
			g_context = nullptr;
		}
	}

	// shutsdown and restarts systems with a new config
	void ChangeSettings(const PlaneverbConfig* newConfig)
	{
		Exit();
		Init(newConfig);
	}

	// sets global listener position
	void SetListenerPosition(const vec3& listenerPosition)
	{
		auto* context = GetContext();
		if(context)
			context->SetListenerPosition(listenerPosition);
	}
	#pragma endregion

	namespace
	{
		// Background thread runs this function
		void BackgroundProcessor(Context* context)
		{
			// get acoustics systems and information
			bool isRunning = context->IsRunning();
			Grid* grid = context->GetGrid();
			GeometryManager* geometry = context->GetGeometryManager();
			Analyzer* analyzer = context->GetAnalyzer();
			const PlaneverbConfig* config = context->GetConfig();
			vec3 listenerPos = context->GetListenerPosition();
			
			// run while context runs
			while (isRunning)
			{
				// debug profile if needed
				PROFILE_SECTION(
				{
					// generate impulse responses
					PROFILE_TIME(grid->GenerateResponse(listenerPos), "Time for Generating Response");
					
					// generate runtime data
					PROFILE_TIME(analyzer->AnalyzeResponses(listenerPos), "Time for Analyzing Response");

					// update geometry in grid
					geometry->PushGeometryChanges();

					// update listener position and running flag
					listenerPos = context->GetListenerPosition();
					isRunning = context->IsRunning();
				}, 
				"Time for one analysis iteration");
			}
		}
	} // namespace <>

	Context::Context(const PlaneverbConfig * config) : 
		m_backgroundProcessor(), m_isRunning(true)
	{
		// throw if input is invalid
		if (config == nullptr || config->gridResolution < pv_LowResolution ||
			config->gridSizeInMeters.x == 0 || config->gridSizeInMeters.y == 0 ||
			config->tempFileDirectory == nullptr || 
			config->maxThreadUsage < 0)
		{
			throw pv_InvalidConfig;
		}

		// copy config
		std::memcpy(&m_config, config, sizeof(PlaneverbConfig));

		// determine size for context pool, throw if operator new fails
		unsigned size = sizeof(GeometryManager) + sizeof(Grid) + sizeof(EmissionManager) + sizeof(Analyzer) + sizeof(FreeGrid);
		m_mem = new char[size];
		if (m_mem == nullptr)
		{
			throw pv_NotEnoughMemory;
		}
		
		// set pool memory to 0
		std::memset(m_mem, 0, size);

		// placement new construct the grid
		m_grid = new (m_mem) Grid(&m_config);

		// placement new construct the geometry manager
		m_geometry = new (m_mem + sizeof(Grid)) GeometryManager(m_grid);

		// placement new construct the emissions manager
		m_emissions = new (m_mem + sizeof(Grid) + sizeof(GeometryManager)) EmissionManager();

		// placement new construct the free grid
		m_freeGrid = new (m_mem + sizeof(Grid) + sizeof(GeometryManager) + sizeof(EmissionManager) + sizeof(Analyzer)) FreeGrid(&m_config);

		// placement new construct the analyzer
		m_analyzer = new (m_mem + sizeof(Grid) + sizeof(GeometryManager) + sizeof(EmissionManager)) Analyzer(m_grid, m_freeGrid);

		// start background thread after all systems are initialized
		m_backgroundProcessor = std::thread(BackgroundProcessor, this);
	}

	Context::~Context()
	{
		// stop the background thread
		StopRunning();
		m_backgroundProcessor.join();

		// call dtor on all systems in reverse order
		m_analyzer->~Analyzer();
		m_emissions->~EmissionManager();
		m_geometry->~GeometryManager();
		m_grid->~Grid();
		m_freeGrid->~FreeGrid();

		// delete pool
		delete[] m_mem;
	}
} // namespace Planeverb
