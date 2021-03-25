#include <FDTD\FreeGrid.h>
#include <Context\PvContext.h>
#include <PvDefinitions.h>
#include <cmath>

namespace Planeverb
{ 
	FreeGrid::FreeGrid(const PlaneverbConfig * config, char* mem) : 
		m_grid(nullptr),
        m_dx(0),
		m_EFree(0.f)
	{
		// make a new temporary grid
		unsigned size = Grid::GetMemoryRequirement(config);
		char* temporaryPool = new char[size];
		if (!temporaryPool)
		{
			throw pv_NotEnoughMemory;
		}
		m_grid = new Grid(config, temporaryPool);
		if (!m_grid)
		{
			throw pv_NotEnoughMemory;
		}

        m_dx = Context::GetGlobals().gridDX;
		PlaneverbConfig simpleConfig = *config;
		simpleConfig.gridCenteringType = pv_StaticCentering;
		simpleConfig.gridWorldOffset = { 0, 0 };
        m_EFree = SimulateFreeFieldEnergy(&simpleConfig);
		
		// delete the temporary grid
		delete m_grid;
		m_grid = nullptr;

		// delete temporary emmory pool.
		delete[] temporaryPool;
		temporaryPool = nullptr;
	}

	FreeGrid::~FreeGrid()
	{
		// grid should be deleted already
	}

	Real FreeGrid::GetEFreePerR(const vec2& listenerPos, vec2& emitterPos)
	{
		// find Euclidean distance between listener and emitter
		Real efree = m_EFree;

		Real r = listenerPos.Distance(emitterPos);
		if (r == 0.f)
		{
			return efree;
		}

		// 2D propagation: energy decays as 1/r
		return efree / r;
	}

    Real FreeGrid::GetEnergyAtOneMeter() const
    {
        return m_EFree;
    }

	unsigned FreeGrid::GetMemoryRequirement(const PlaneverbConfig * config)
	{
		return 0;
	}

	Real FreeGrid::SimulateFreeFieldEnergy(const PlaneverbConfig* config)
	{
		const auto& globals = Context::GetGlobals();

		vec3 listenerWorldPos3 = { 0, 0, 0 };
		vec2 emitterWorldPos = { 1, 0 };

		// generate a set of IRs in the grid, calculate the free energy
		m_grid->GenerateResponse(listenerWorldPos3);
		const Cell* response = m_grid->GetResponse(emitterWorldPos);
        Real freeFieldEnergy = CalculateEFree(response, globals.responseSampleLength, globals.samplingRate);

        // discrete distance on grid
		int emitterX, emitterY;
		int listenerX, listenerY;
		vec2 listenerWorldPos2 = { listenerWorldPos3.x, listenerWorldPos3.z };
		m_grid->WorldToGrid(emitterWorldPos, emitterX, emitterY);
		m_grid->WorldToGrid(listenerWorldPos2, listenerX, listenerY);

        const Real r = Real(emitterX - listenerX) * m_dx;
        // Normalize to exactly 1m assuming 1/r energy attenuation
        freeFieldEnergy *= r;

        return freeFieldEnergy;
	}

	Real FreeGrid::CalculateEFree(const Cell * response, int responseLength, Real samplingRate) const
	{
		// Dry duration, plus delay to get 1m away
        int numSamples = (int)((PV_DRY_GAIN_ANALYSIS_LENGTH) * samplingRate) + (int)((Real(1) / PV_C) * samplingRate);
		PV_ASSERT(numSamples < responseLength);
		Real efree = 0.f;

		// sum up square of signal values
		for (int i = 0; i < numSamples; ++i)
		{
			efree += response[i].pr * response[i].pr;
		}

		return efree;
	}
} // namespace Planeverb
