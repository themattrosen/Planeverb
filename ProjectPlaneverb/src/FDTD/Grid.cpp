#include <FDTD\Grid.h>
#include <PvDefinitions.h>
#include <Context\PvContext.h>
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
		m_pulse(nullptr)
	{
		// calculate internals
		auto& globals = Context::GetGlobals();
		CalculateGridParameters(config->gridResolution, globals.gridDX, globals.simulationDT, globals.samplingRate);

		globals.gridSize.x = (1.f / globals.gridDX) * globals.config.gridSizeInMeters.x;
		globals.gridSize.y = (1.f / globals.gridDX) * globals.config.gridSizeInMeters.y;

		// calculate total memory size
		// length per grid uses gridsize + 1 for extended velocity fields
		unsigned lengthPerGrid = (unsigned)(globals.gridSize.x + 1) * (unsigned)(globals.gridSize.y + 1);
		unsigned lengthPerResponse = (unsigned)(globals.samplingRate * globals.impulseResponseTimeS); 
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

		vec2 incGridSize(globals.gridSize.x + 1, globals.gridSize.y + 1);
		globals.responseSampleLength = lengthPerResponse;

		// init the b and by field
		int numBIterations = (int)incGridSize.x * (int)incGridSize.y;
		for (int i = 0; i < numBIterations; ++i)
		{
			int row = i / (int)incGridSize.y;
			int col = i % (int)incGridSize.y;
			Cell& nextCell = m_grid[i];
			if (row == (int)globals.gridSize.x || col == (int)globals.gridSize.y)
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
		GaussianPulse(config, Real(globals.samplingRate), m_pulse, globals.responseSampleLength);
	}

	Grid::~Grid()
	{
		if (m_mem)
		{
			const auto& globals = Context::GetGlobals();
			int loopSize = (int)(globals.gridSize.x + 1) * (int)(globals.gridSize.y + 1);
			// destruct each vector
			for (int i = 0; i < loopSize; ++i)
			{
				m_pulseResponse[i].clear();
				m_pulseResponse[i].shrink_to_fit();
				//TODO: calling ~vector here was failing
				// instead just making sure all dynamically allocated memory is freed
			}

			// delete the pool
			//delete[] m_mem;
			//m_mem = nullptr;
		}
	}

	void Grid::AddAABB(const AABB * transform)
	{
		const auto& globals = Context::GetGlobals();

		// define edges of the AABB
		int startY;
		int startX;
		int endY;
		int endX;

		int gridSizex = (int)globals.gridSize.x;
		int gridSizey = (int)globals.gridSize.y;
		
		WorldToGrid({ transform->position.x - transform->width / (Real)2.f ,
			transform->position.y - transform->height / (Real)2.f },
			startX, startY);

		WorldToGrid({ transform->position.x + transform->width / (Real)2.f ,
			transform->position.y + transform->height / (Real)2.f },
			endX, endY);

		const vec2 newGridSize(globals.gridSize.x + 1, globals.gridSize.y + 1);

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
			if (i >= 0 && i <= gridSizey)
			{
				for (int j = startX; j < endX; ++j)
				{
					if (j >= 0 && j <= gridSizex)
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
		const auto& globals = Context::GetGlobals();

		// define edges of the AABB
		int startY;
		int startX;
		int endY;  
		int endX; 

		int gridSizex = (int)globals.gridSize.x;
		int gridSizey = (int)globals.gridSize.y;

		WorldToGrid({ transform->position.x - transform->width / (Real)2.f , 
			transform->position.y - transform->height / (Real)2.f },
			startX, startY);

		WorldToGrid({ transform->position.x + transform->width / (Real)2.f ,
			transform->position.y + transform->height / (Real)2.f },
			endX, endY);


		vec2 newGridSize(Real(gridSizex + 1), Real(gridSizey + 1));

		// reset area of the AABB
		for (int i = startY; i < endY; ++i)
		{
			if (i >= 0 && i <= gridSizey)
			{
				for (int j = startX; j < endX; ++j)
				{
					if (j >= 0 && j <= gridSizex)
					{
						int index = INDEX(j, i, newGridSize);
						Cell& nextCell = m_grid[index];
						nextCell.absorption = PV_ABSORPTION_FREE_SPACE;
						
						nextCell.b = 1;
						nextCell.by = 1;
						
						if (i == gridSizex || j == gridSizey)
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
		const auto& globals = Context::GetGlobals();
		int gridSizex = (int)globals.gridSize.x;
		int gridSizey = (int)globals.gridSize.y;
		const int numBIterations = (gridSizex + 1) * (gridSizey + 1);
		const int ydim = gridSizey + 1;
		const int MAX_ROWS = gridSizex;
		const int MAX_COLS = gridSizey;

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
		const auto& globals = Context::GetGlobals();
		vec2 listenerPos{ globals.listenerPos.x, globals.listenerPos.z };
		const vec2 gridDimensions = globals.config.gridSizeInMeters;
		const vec2 gridOffset = globals.config.gridWorldOffset;

		if (globals.config.gridCenteringType == pv_DynamicCentering)
		{
			vec2 toPlayerSpace = 
			{
				worldspace.x - listenerPos.x + gridDimensions.x / Real(2.0) - gridOffset.x,
				worldspace.y - listenerPos.y + gridDimensions.y / Real(2.0) - gridOffset.y
			};

			gridx = (int)(toPlayerSpace.x / globals.gridDX);
			gridy = (int)(toPlayerSpace.y / globals.gridDX);
		}
		else
		{
			vec2 toStaticSpace =
			{
				worldspace.x + gridDimensions.x / Real(2.0) - gridOffset.x,
				worldspace.y + gridDimensions.y / Real(2.0) - gridOffset.y
			};
			
			gridx = (int)(toStaticSpace.x / globals.gridDX);
			gridy = (int)(toStaticSpace.y / globals.gridDX);
		}
	}

	void Grid::GridToWorld(int gridx, int gridy, vec2 & worldspace) const
	{
		const auto& globals = Context::GetGlobals();
		vec2 listenerPos{ globals.listenerPos.x, globals.listenerPos.z };
		const vec2 gridDimensions = globals.config.gridSizeInMeters;
		const vec2 gridOffset = globals.config.gridWorldOffset;
		vec2 to =
		{
			(Real)gridx * globals.gridDX,
			(Real)gridy * globals.gridDX
		};

		if (globals.config.gridCenteringType == pv_DynamicCentering)
		{
			worldspace.x = to.x + listenerPos.x - gridDimensions.x / Real(2.0) + gridOffset.x;
			worldspace.y = to.y + listenerPos.y - gridDimensions.y / Real(2.0) + gridOffset.y;
		}
		else
		{
			worldspace.x = to.x - gridDimensions.x / Real(2.0) + gridOffset.x;
			worldspace.y = to.y - gridDimensions.y / Real(2.0) + gridOffset.y;
		}

	}

	// Debug print the grid
	void Grid::PrintGrid()
	{
		const auto& globals = Context::GetGlobals();
		int gridx = (int)globals.gridSize.x + 1;
		int gridy = (int)globals.gridSize.y + 1;
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
		auto& globals = Context::GetGlobals();
		vec2 m_gridOffset = config->gridWorldOffset;

		Real m_dx, m_dt;
		CalculateGridParameters(config->gridResolution, m_dx, m_dt, globals.samplingRate);

		vec2 m_gridSize;
		m_gridSize.x = (1.f / m_dx) * config->gridSizeInMeters.x;
		m_gridSize.y = (1.f / m_dx) * config->gridSizeInMeters.y;

		// calculate total memory size
		// length per grid uses gridsize + 1 for extended velocity fields
		unsigned lengthPerGrid = (unsigned)(m_gridSize.x + 1) * (unsigned)(m_gridSize.y + 1);
		unsigned lengthPerResponse = (unsigned)(globals.samplingRate * globals.impulseResponseTimeS);
		unsigned size =
			lengthPerResponse * sizeof(Real) +	// memory for Gaussian pulse values
			lengthPerGrid * sizeof(Cell) +		// memory for Cell grid

			/// memory for pulse response Cell[x][y][t]
			///sizePerGrid * lengthPerResponse;
			// memory for pulse response std::vector<Cell>[x][y]
			lengthPerGrid * sizeof(std::vector<Cell>);

		return size;
	}

	void CalculateGridParameters(int resolution, Real & dx, Real & dt, Real &samplingRate)
	{
		Real minWavelength = PV_C / (Real)resolution;
		dx = minWavelength / PV_POINTS_PER_WAVELENGTH;
		dt = dx / (PV_C * Real(1.5));
		samplingRate = (Real(1.0) / dt);
	}
} // namespace Planeverb
