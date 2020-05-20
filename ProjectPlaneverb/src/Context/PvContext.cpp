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
		unsigned systemSize = sizeof(GeometryManager) + sizeof(Grid) + sizeof(EmissionManager) + sizeof(Analyzer) + sizeof(FreeGrid);
		unsigned internalSize = GeometryManager::GetMemoryRequirement(config) +
			Grid::GetMemoryRequirement(config) +
			EmissionManager::GetMemoryRequirement(config) +
			Analyzer::GetMemoryRequirement(config) +
			FreeGrid::GetMemoryRequirement(config);
		unsigned size = systemSize + internalSize;
		m_systemMem = new char[size];
		if (m_systemMem == nullptr)
		{
			throw pv_NotEnoughMemory;
		}

		m_mem = m_systemMem + systemSize;
		
		char* tempSysMem = m_systemMem;
		char* tempPoolMem = m_mem;

		// set pool memory to 0
		std::memset(m_systemMem, 0, size);

		// placement new construct the grid
		m_grid = new (tempSysMem) Grid(&m_config, tempPoolMem);
		tempSysMem += sizeof(Grid);
		tempPoolMem += Grid::GetMemoryRequirement(config);

		// placement new construct the geometry manager
		m_geometry = new (tempSysMem) GeometryManager(m_grid, tempPoolMem);
		tempSysMem += sizeof(GeometryManager);
		tempPoolMem += GeometryManager::GetMemoryRequirement(config);

		// placement new construct the emissions manager
		m_emissions = new (tempSysMem) EmissionManager(tempPoolMem);
		tempSysMem += sizeof(EmissionManager);
		tempPoolMem += EmissionManager::GetMemoryRequirement(config);

		// placement new construct the free grid
		m_freeGrid = new (tempSysMem) FreeGrid(&m_config, tempPoolMem);
		tempSysMem += sizeof(FreeGrid);
		tempPoolMem += FreeGrid::GetMemoryRequirement(config);

		// placement new construct the analyzer
		m_analyzer = new (tempSysMem) Analyzer(m_grid, m_freeGrid, tempPoolMem);
		tempSysMem += sizeof(Analyzer);
		tempPoolMem += Analyzer::GetMemoryRequirement(config);

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
		delete[] m_systemMem;
		//delete[] m_mem; implied
	}
} // namespace Planeverb
