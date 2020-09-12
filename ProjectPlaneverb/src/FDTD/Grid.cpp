#include <FDTD\Grid.h>
#include <PvDefinitions.h>
#include <cmath>
#include <cstring>
#include <iostream>

namespace Planeverb
{
	namespace
	{
		// Fill a given array with a precomputed Gaussian pulse
		void GaussianPulse(const PlaneverbConfig* config, Real samplingRate, Real* out, unsigned numSamples)
		{
            const Real maxFreq = Real(config->gridResolution);
            const Real pi = std::acos(Real(-1));
            const Real sigma = Real(1.0f) / (Real(0.5) * pi * maxFreq);

			const Real delay = Real(2.0) * sigma;
            const Real dt = Real(1.0) / samplingRate;

            for (unsigned i = 0; i < numSamples; ++i)
			{
				Real t = (Real)i * dt;
				Real val = std::exp(-(t - delay) * (t - delay) / (sigma * sigma));
				*out++ = val;
			}
		}
	} // namespace <>

	Grid::Grid(const PlaneverbConfig* config, char* mem) :
		m_mem(mem),
		m_grid(nullptr),
		m_pulseResponse(nullptr),
		m_pulse(nullptr),
		m_dx(), m_dt(),
		m_gridSize(), m_gridDimensions(config->gridSizeInMeters), m_gridOffset(), m_responseLength(),
		m_samplingRate(),
		m_resolution(config->gridResolution),
		m_executionType(config->threadExecutionType),
		m_maxThreads(config->maxThreadUsage),
		m_centering(config->gridCenteringType)
	{
		// calculate internals
		m_gridOffset = config->gridWorldOffset;

		CalculateGridParameters(config->gridResolution, m_dx, m_dt, m_samplingRate);

		m_gridSize.x = (1.f / m_dx) * m_gridDimensions.x;
		m_gridSize.y = (1.f / m_dx) * m_gridDimensions.y;

		// calculate total memory size
		// length per grid uses gridsize + 1 for extended velocity fields
		unsigned lengthPerGrid = (unsigned)(m_gridSize.x + 1) * (unsigned)(m_gridSize.y + 1);
		unsigned lengthPerResponse = (unsigned)(m_samplingRate * PV_IMPULSE_RESPONSE_S); 
		unsigned size =
			lengthPerResponse * sizeof(Real) +	// memory for Gaussian pulse values
			lengthPerGrid * sizeof(Cell) +		// memory for Cell grid

			/// memory for pulse response Cell[x][y][t]
			///sizePerGrid * lengthPerResponse;
			// memory for pulse response std::vector<Cell>[x][y]
			lengthPerGrid * sizeof(std::vector<Cell>);

		// allocate memory pool, throw for operator new fails. set memory to zero
		if (!m_mem)
		{
			throw pv_NotEnoughMemory;
		}
		std::memset(m_mem, 0, size);

		// set grids and arrays offset into pool
		char* temp = m_mem;
		m_pulse = reinterpret_cast<Real*>(temp);				temp += lengthPerResponse * sizeof(Real);
		m_grid = reinterpret_cast<Cell*>(temp);					temp += lengthPerGrid * sizeof(Cell);
		m_pulseResponse = reinterpret_cast<std::vector<Cell>*>(temp);

		vec2 incGridSize(m_gridSize.x + 1, m_gridSize.y + 1);
		m_responseLength = lengthPerResponse;

		// init the b and by field
		int numBIterations = (int)incGridSize.x * (int)incGridSize.y;
		for (int i = 0; i < numBIterations; ++i)
		{
			int row = i / (int)incGridSize.y;
			int col = i % (int)incGridSize.y;
			Cell& nextCell = m_grid[i];
			if (row == (int)m_gridSize.x || col == (int)m_gridSize.y)
			{
				nextCell.b = 0;
				nextCell.by = 0;
			}
			else if (col == 0)
			{
				nextCell.b = 1;
				nextCell.by = 0;
			}
			else
			{
				nextCell.b = 1;
				nextCell.by = 1;
			}

			// initialize pulseResponse
			new (&m_pulseResponse[i]) std::vector<Cell>(); // placement new to call ctor
			m_pulseResponse[i].resize(lengthPerResponse, Cell());
		}

		// precompute Gaussian pulse
		GaussianPulse(config, Real(m_samplingRate), m_pulse, m_responseLength);
	}

	Grid::~Grid()
	{
		if (m_mem)
		{
			int loopSize = (int)(m_gridSize.x + 1) * (int)(m_gridSize.y + 1);
			// destruct each vector
			for (int i = 0; i < loopSize; ++i)
			{
				m_pulseResponse[i].~vector();
			}

			// delete the pool
			//delete[] m_mem;
			//m_mem = nullptr;
		}
	}

	void Grid::AddAABB(const AABB * transform)
	{
		// define edges of the AABB
		int startY;
		int startX;
		int endY;
		int endX;
		
		WorldToGrid({ transform->position.x - transform->width / (Real)2.f ,
			transform->position.y - transform->height / (Real)2.f },
			startX, startY);

		WorldToGrid({ transform->position.x + transform->width / (Real)2.f ,
			transform->position.y + transform->height / (Real)2.f },
			endX, endY);

		const vec2 newGridSize(m_gridSize.x + 1, m_gridSize.y + 1);

		/*
		// top
		if (startY >= 0 && startY < m_gridSize.y)
		{
			for (int i = startX; i < endX - 1; ++i)
			{
				if (i >= 0 && i < m_gridSize.x)
				{
					int index = INDEX(i, startY, newGridSize);
					m_boundaries[index].normal = vec2(-1, 0);
					m_boundaries[index].absorption = transform->absorption;
					m_grid[index].b = 0;
					m_grid[index].by = 0;
				}
			}
		}
		// bottom
		if (endY >= 0 && endY < m_gridSize.y)
		{
			for (int i = startX; i < endX; ++i)
			{
				if (i >= 0 && i < m_gridSize.x)
				{
					int index = INDEX(i, endY - 1, newGridSize);
					m_boundaries[index].normal = vec2(1, 0);
					m_boundaries[index].absorption = transform->absorption;
					m_grid[index].b = 0;
					m_grid[index].by = 0;
				}
			}
		}

		// left
		if (startX >= 0 && startX < m_gridSize.x)
		{
			for (int i = startY; i < endY; ++i)
			{
				if (i >= 0 && i < m_gridSize.y)
				{
					int index = INDEX(startX, i, newGridSize);
					m_boundaries[index].normal = vec2(0, -1);
					m_boundaries[index].absorption = transform->absorption;
					m_grid[index].b = 0;
					m_grid[index].by = 0;
				}
			}
		}
		// right
		if (endX >= 0 && endX < m_gridSize.x)
		{
			for (int i = startY; i < endY; ++i)
			{
				if (i >= 0 && i < m_gridSize.y)
				{
					int index = INDEX(endX - 1, i, newGridSize);
					m_boundaries[index].normal = vec2(0, 1);
					m_boundaries[index].absorption = transform->absorption;
					m_grid[index].b = 0;
					m_grid[index].by = 0;
				}
			}
		}

		// inside
		for (int i = startY + 1; i < endY - 1; ++i)
		{
			if (i >= 0 && i < m_gridSize.y)
			{
				for (int j = startX + 1; j < endX - 1; ++j)
				{
					if (j >= 0 && j < m_gridSize.x)
					{
						int index = INDEX(j, i, newGridSize);
						m_boundaries[index].normal = vec2(0, 0);
						m_boundaries[index].absorption = PV_ABSORPTION_FREE_SPACE;
						m_grid[index].b = 0;
						m_grid[index].by = 0;
					}
				}
			}
		}
		*/

		for (int i = startY; i < endY; ++i)
		{
			if (i >= 0 && i <= m_gridSize.y)
			{
				for (int j = startX; j < endX; ++j)
				{
					if (j >= 0 && j <= m_gridSize.x)
					{
						int index = INDEX(j, i, newGridSize);
						Cell& nextCell = m_grid[index];
						nextCell.absorption = transform->absorption;
						nextCell.b = 0;
						nextCell.by = 0;
					}
				}
			}
		}
	}

	void Grid::RemoveAABB(const AABB * transform)
	{
		// define edges of the AABB
		int startY;
		int startX;
		int endY;  
		int endX; 

		WorldToGrid({ transform->position.x - transform->width / (Real)2.f , 
			transform->position.y - transform->height / (Real)2.f },
			startX, startY);

		WorldToGrid({ transform->position.x + transform->width / (Real)2.f ,
			transform->position.y + transform->height / (Real)2.f },
			endX, endY);


		vec2 newGridSize(m_gridSize.x + 1, m_gridSize.y + 1);

		// reset area of the AABB
		for (int i = startY; i < endY; ++i)
		{
			if (i >= 0 && i <= m_gridSize.y)
			{
				for (int j = startX; j < endX; ++j)
				{
					if (j >= 0 && j <= m_gridSize.x)
					{
						int index = INDEX(j, i, newGridSize);
						Cell& nextCell = m_grid[index];
						nextCell.absorption = PV_ABSORPTION_FREE_SPACE;
						
						nextCell.b = 1;
						nextCell.by = 1;
						
						if (i == (int)m_gridSize.x || j == (int)m_gridSize.y)
						{
							nextCell.b = 0;
							nextCell.by = 0;
						}
						else if (j == 0)
						{
							nextCell.b = 1;
							nextCell.by = 0;
						}
						else
						{
							nextCell.b = 1;
							nextCell.by = 1;
						}
					}
				}
			}
		}
	}

	void Grid::ClearAABBs()
	{
		const int numBIterations = (int)((m_gridSize.x + 1) * (m_gridSize.y + 1));
		const int ydim = (int)(m_gridSize.y + 1);
		const int MAX_ROWS = (int)m_gridSize.x;
		const int MAX_COLS = (int)m_gridSize.y;

		Cell* gridArr = m_grid;
		for (int i = 0; i < numBIterations; ++i)
		{
			int row = i / (int)ydim;
			int col = i % (int)ydim;

			Cell& nextCell = *gridArr++;
			nextCell.absorption = PV_ABSORPTION_FREE_SPACE;
			nextCell.b = 1;
			nextCell.by = 1;
		}

		// fill out bottom extra edge
		for (int i = 0; i < MAX_COLS; ++i)
		{
			int index = INDEX2(MAX_ROWS, i, ydim);
			Cell& nextCell = m_grid[index];
			nextCell.b = 0;
			nextCell.by = 0;
		}

		// fill out right extra edge
		for (int i = 0; i < MAX_ROWS; ++i)
		{
			int index = INDEX2(i, MAX_COLS, ydim);
			Cell& nextCell = m_grid[index];
			nextCell.b = 0;
			nextCell.by = 0;
		}

		// fill out FIRST column for by particle velocity
		for (int i = 0; i < MAX_ROWS; ++i)
		{
			int index = INDEX2(i, 0, ydim);
			Cell& nextCell = m_grid[index];
			nextCell.by = 0;
		}
	}

	void Grid::WorldToGrid(const vec2 & worldspace, int & gridx, int & gridy) const
	{
		if (m_centering == pv_DynamicCentering)
		{
			//TODO
		}
		else
		{
			vec2 toStaticSpace =
			{
				worldspace.x + m_gridDimensions.x / Real(2.0) + m_gridOffset.x,
				worldspace.y + m_gridDimensions.y / Real(2.0) + m_gridOffset.y
			};
			
			gridx = (int)(toStaticSpace.x / m_dx);
			gridy = (int)(toStaticSpace.y / m_dx);
		}
	}

	// Debug print the grid
	void Grid::PrintGrid()
	{
		int gridx = (int)m_gridSize.x + 1;
		int gridy = (int)m_gridSize.y + 1;
		vec2 newGridSize((Real)gridx, (Real)gridy);

		for (int i = 0; i < gridx - 1; ++i)
		{
			for (int j = 0; j < gridy - 1; ++j)
			{
				int index = INDEX(i, j, newGridSize);

				const Cell& cell = m_grid[index];
				if (cell.b || cell.by)
				{
					std::cout << " .";
				}
				else
				{
					std::cout << "00";
				}

			}

			std::cout << '\n';
		}

		std::cout << '\n';
	}

	unsigned Grid::GetMemoryRequirement(const PlaneverbConfig * config)
	{
		// calculate internals
		vec2 m_gridOffset = config->gridWorldOffset;

		Real m_dx, m_dt;
		unsigned m_samplingRate;
		CalculateGridParameters(config->gridResolution, m_dx, m_dt, m_samplingRate);

		vec2 m_gridSize;
		m_gridSize.x = (1.f / m_dx) * config->gridSizeInMeters.x;
		m_gridSize.y = (1.f / m_dx) * config->gridSizeInMeters.y;

		// calculate total memory size
		// length per grid uses gridsize + 1 for extended velocity fields
		unsigned lengthPerGrid = (unsigned)(m_gridSize.x + 1) * (unsigned)(m_gridSize.y + 1);
		unsigned lengthPerResponse = (unsigned)(m_samplingRate * PV_IMPULSE_RESPONSE_S);
		unsigned size =
			lengthPerResponse * sizeof(Real) +	// memory for Gaussian pulse values
			lengthPerGrid * sizeof(Cell) +		// memory for Cell grid

			/// memory for pulse response Cell[x][y][t]
			///sizePerGrid * lengthPerResponse;
			// memory for pulse response std::vector<Cell>[x][y]
			lengthPerGrid * sizeof(std::vector<Cell>);

		return size;
	}

	void CalculateGridParameters(int resolution, Real & dx, Real & dt, unsigned & samplingRate)
	{
		Real minWavelength = PV_C / (Real)resolution;
		dx = minWavelength / PV_POINTS_PER_WAVELENGTH;
		dt = dx / (PV_C * Real(1.5));
		samplingRate = (unsigned)(Real(1.0) / dt);
	}
} // namespace Planeverb
