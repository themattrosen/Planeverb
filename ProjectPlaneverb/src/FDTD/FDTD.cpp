#include <FDTD\Grid.h>
#include <Planeverb.h>
#include <PvDefinitions.h>

#include <Context\PvContext.h>

#include <DSP\Analyzer.h>
#include <Emissions\EmissionManager.h>
#include <Util/ScopedTimer.h>
#include <omp.h>
#include <iostream>

namespace Planeverb
{
#pragma region ClientInterface
	PlaneverbOutput GetOutput(EmissionID emitter)
	{
		PlaneverbOutput out;
		std::memset(&out, 0, sizeof(out));
		auto* context = GetContext();

		// case module hasn't been created yet
		if(!context)
		{
			out.occlusion = PV_INVALID_DRY_GAIN;
			return out;
		}

		auto* analyzer = context->GetAnalyzer();
		auto* emissions = context->GetEmissionManager();
		const auto* emitterPos = emissions->GetEmitter(emitter);

		// case emitter is invalid
		if (!emitterPos)
		{
			out.occlusion = PV_INVALID_DRY_GAIN;
			return out;
		}

		auto* result = analyzer->GetResponseResult(*emitterPos);

		// case invalid emitter position
		if (!result)
		{
			out.occlusion = PV_INVALID_DRY_GAIN;
			return out;
		}

		// copy over values
		out.occlusion = (float)result->occlusion;
		out.lowpass = (float)result->lowpassIntensity;
		out.rt60 = (float)result->rt60;
		out.direction = result->direction;

		return out;
	}

#pragma endregion
	
	Cell* Grid::GetResponse(const vec2& gridPosition)
	{
		vec2 incDim(m_gridSize.x + 1, m_gridSize.y + 1);
		int index = INDEX((int)gridPosition.x, (int)gridPosition.y, incDim);
		return m_pulseResponse[index].data();
	}

	unsigned Grid::GetResponseSize() const
	{
		return m_responseLength;
	}
	
	// process FDTD
	void Grid::GenerateResponseCPU(const vec3 & listener)
	{
		// determine pressure and velocity update constants
		const Real Courant = m_dt / m_dx;
		const Real Cv = Courant / PV_RHO;
		const Real Cprv = Courant * PV_RHO * PV_C * PV_C;

		// grid constants
		const int gridx = (int)m_gridSize.x;
		const int gridy = (int)m_gridSize.y;
		const vec2 dim = m_gridSize;
		const vec2 incdim(dim.x + 1, dim.y + 1);
		const int listenerPosX = (int)((listener.x + m_gridOffset.x) / m_dx);
		const int listenerPosY = (int)((listener.z + m_gridOffset.y) / m_dx);
		const int listenerPos = listenerPosX * (gridy + 1) + listenerPosY;
		const int responseLength = m_responseLength;
		int loopSize = (int)(incdim.x) * (int)(incdim.y);

		// thread usage
		if (m_maxThreads == 0)
			omp_set_num_threads(omp_get_max_threads());
		else
			omp_set_num_threads(m_maxThreads);

		// RESET all pressure and velocity, but not B fields (can't use memset)
		{
			Cell* resetPtr = m_grid;
			for (int i = 0; i < loopSize; ++i)
			{
				resetPtr->pr = 0.f;
				resetPtr->vx = 0.f;
				resetPtr->vy = 0.f;
				++resetPtr;
			}
		}

		// process FDTD update for 1 IR
		for (int t = 0; t < responseLength; ++t)
		{
			// process pressure grid
			{
				#pragma omp parallel for
				for (int i = 0; i < loopSize; ++i)
				{
					vec2* normal = &(m_boundaries[i].normal);
					int normalInd = i + (int)normal->y * (gridy + 1) + (int)normal->x;	// [i + n.x, j + n.y]
					Cell thisCell = m_grid[i];
					int B = (int)thisCell.b;
					Real R = m_boundaries[i].absorption;
					Real absorption = (1.f - R) / (1.f + R);

					Cell nextCellX = m_grid[(i + gridy + 1) * B];	// [i + 1, j]
					Cell nextCellY = m_grid[(i + 1) * B];			// [i, j + 1]

					m_grid[i].pr = m_grid[normalInd].pr * absorption -
						Cprv * (Real)B * ((nextCellX.vx - thisCell.vx) + (nextCellY.vy - thisCell.vy));
				}
			}

			// process x component of particle velocity
			{
				// eq to for(1 to sizex) for(0 to sizey)
				#pragma omp parallel for
				for (int i = gridy + 1; i < loopSize; ++i)
				{
					vec2* normal = &(m_boundaries[i].normal);
					int normalInd = i + (int)normal->y * (gridy + 1) + (int)normal->x;	// [i + n.x, j + n.y]
					Cell thisCell = m_grid[i];											// [i, j]
					int B = (int)thisCell.b;
					Cell nextCell = m_grid[(i - gridy - 1) * B];						// [i - 1, j]
					Real R = m_boundaries[i].absorption;
					Real absorption = (1.f - R) / (1.f + R);

					m_grid[i].vx = m_grid[normalInd].vx * absorption -
						Cv * (Real)B * (thisCell.pr - nextCell.pr);
				}
			}

			// process y component of particle velocity
			{
				// eq to for(0 to sizex) for(1 to sizey)
				#pragma omp parallel for
				for (int i = 1; i < loopSize; ++i)
				{
					vec2 normal = (m_boundaries[i].normal);
					int normalInd = i + (int)normal.y * (gridy + 1) + (int)normal.x;	// [i + n.x, j + n.y]
					Cell thisCell = m_grid[i];											// [i, j]
					int B = thisCell.by;
					Cell nextCell = m_grid[(i - 1) * B];								// [i, j - 1]
					Real R = m_boundaries[i].absorption;
					Real absorption = (1.f - R) / (1.f + R);

					m_grid[i].vy = m_grid[normalInd].vy * absorption -
						Cv * (Real)B * (thisCell.pr - nextCell.pr);
				}
			}

			// process ABC top/bottom
			{
				#pragma omp parallel for
				for (int i = 0; i < gridy; ++i)
				{
					int index1 = i;
					int index2 = gridx * (gridy + 1) + i;

					m_grid[index1].vx = -m_grid[index1].pr * m_z_inv;
					m_grid[index2].vx = m_grid[index2 - gridy - 1].pr * m_z_inv;
				}
			}

			// process ABC left/right
			{
				#pragma omp parallel for
				for (int i = 0; i < gridx; ++i)
				{
					int index1 = i * (gridy + 1);
					int index2 = i * (gridy + 1) + gridy;

					m_grid[index1].vy = -m_grid[index1].pr * m_z_inv;
					m_grid[index2].vy = m_grid[index2 - 1].pr * m_z_inv;
				}
			}

			// add results to the response cube
			{
				#pragma omp parallel for
				for (int i = 0; i < loopSize; ++i)
				{
					m_pulseResponse[i][t] = m_grid[i];
				}
			}

			// add pulse to listener position pressure field
			m_grid[listenerPos].pr += m_pulse[t];
		}
	}

	void Grid::GenerateResponseGPU(const vec3& listener)
	{
		// not currently supported
		throw pv_InvalidConfig;
	}

	void Grid::GenerateResponse(const vec3& listener)
	{
		if (m_executionType == PlaneverbExecutionType::pv_CPU)
		{
			GenerateResponseCPU(listener);
		}
		else
		{
			GenerateResponseGPU(listener);
		}
	}
} // namespace Planeverb
