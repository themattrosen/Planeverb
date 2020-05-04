#include <FDTD\FreeGrid.h>
#include <PvDefinitions.h>

/*
	Free Grid file spec:
	File Name: .free_responseX<meters.x>X<meters.y>X<scale>
		example: .free_responseX10X10X0.1

	File Contents:
		<EFree of the grid>

	That's it. Just one value.
*/

namespace Planeverb
{ 
	FreeGrid::FreeGrid(const PlaneverbConfig * config) : 
		m_grid(nullptr),
		m_EFree(0.f)
	{
		// make a new temporary grid
		m_grid = new Grid(config);
		if (!m_grid)
		{
			throw pv_NotEnoughMemory;
		}

		// construct file name
		m_dx = m_grid->GetDX();
		std::string fileString(config->tempFileDirectory);
		fileString += "\\free_responseX";
		fileString += std::to_string(config->gridSizeInMeters.x);
		fileString += "X";
		fileString += std::to_string(config->gridSizeInMeters.y);
		fileString += "X";
		fileString += std::to_string(config->gridResolution);
		fileString += ".dat";

		// try to open free energy file
		std::ifstream fileForRead(fileString.c_str());

		// case no existing file, generate it
		if (!fileForRead.is_open())
		{
			GenerateFile(config, fileString);
		}
		// case existing file, use it
		else
		{
			ParseFile(config, fileForRead);
		}

		// delete the temporary grid
		delete m_grid;
		m_grid = nullptr;
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

		// divide free energy by distance
		return efree / r;
	}

	void FreeGrid::ParseFile(const PlaneverbConfig* config, std::ifstream & file)
	{
		// retrieve the free energy
		file >> m_EFree;
	}

	void FreeGrid::GenerateFile(const PlaneverbConfig* config, const std::string & fileName)
	{
		vec2 gridScale = m_grid->GetGridSize();
		int gridx = (int)gridScale.x;
		int gridy = (int)gridScale.y;
		int sizePerGrid = gridx * gridy;
		
		// open a new file for write, throw if can't create a file in the given directory
		std::fstream of(fileName.c_str(), std::ios_base::out);
		if (!of.is_open())
		{
			throw pv_InvalidConfig;
		}

		int listenerX = gridx / 2;
		int listenerY = gridy / 2;
		int emitterX = listenerX + (int)(1.f / m_dx);
		int emitterY = listenerY;

		// generate a set of IRs in the grid, calculate the free energy
		m_grid->GenerateResponse(vec3(listenerX * m_dx, 0, listenerY * m_dx));
		const Cell* response = m_grid->GetResponse(vec2((float)emitterX, (float)emitterY));
		m_EFree = CalculateEFree(response, m_grid->GetResponseSize(), 
			listenerX, listenerY, (int)m_grid->GetSamplingRate());

		// add data to the file
		of << m_EFree;
	}

	Real FreeGrid::CalculateEFree(const Cell * response, int responseLength,
		int listenerX, int listenerY, int samplingRate) const
	{
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
