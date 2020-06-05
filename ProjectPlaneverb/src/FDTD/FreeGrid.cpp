#include <FDTD\FreeGrid.h>
#include <PvDefinitions.h>

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

        m_dx = m_grid->GetDX();
        m_EFree = SimulateFreeFieldEnergy(config);
		
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

	Real FreeGrid::GetEFreePerR(int listenerIndX, int listenerIndY, int emitterIndX, int emitterIndY)
	{
		// find Euclidean distance between listener and emitter
		Real efree = m_EFree;
		Real lX = (Real)listenerIndX * m_dx;
		Real lY = (Real)listenerIndY * m_dx;
		Real eX = (Real)emitterIndX * m_dx;
		Real eY = (Real)emitterIndY * m_dx;

		Real r = std::sqrt((eX - lX) * (eX - lX) +
			(eY - lY) * (eY - lY));
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
		vec2 gridScale = m_grid->GetGridSize();
		int gridx = (int)gridScale.x;
		int gridy = (int)gridScale.y;
		int sizePerGrid = gridx * gridy;
		
		int listenerX = gridx / 2;
		int listenerY = gridy / 2;
		int emitterX = listenerX + (int)(1.f / m_dx);
		int emitterY = listenerY;

		// generate a set of IRs in the grid, calculate the free energy
		m_grid->GenerateResponse(vec3(listenerX * m_dx, 0, listenerY * m_dx));
		const Cell* response = m_grid->GetResponse(vec2((float)emitterX, (float)emitterY));
        Real freeFieldEnergy = CalculateEFree(response, m_grid->GetResponseSize(), (int)m_grid->GetSamplingRate());

        // discrete distance on grid
        const Real r = Real(emitterX - listenerX) * m_dx;
        // Normalize to exactly 1m assuming 1/r energy attenuation
        freeFieldEnergy *= r;

        return freeFieldEnergy;
	}

	Real FreeGrid::CalculateEFree(const Cell * response, int responseLength, int samplingRate) const
	{
		// Dry duration, plus delay to get 1m away
        int numSamples = (int)((PV_DRY_GAIN_ANALYSIS_LENGTH) * ((Real)samplingRate)) + (int)(((Real)1.f / PV_C) * (Real)samplingRate);
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
