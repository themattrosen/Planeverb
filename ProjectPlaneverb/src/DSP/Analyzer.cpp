#include <DSP\Analyzer.h>
#include <FDTD\Grid.h>
#include <FDTD\FreeGrid.h>
#include <PvDefinitions.h>

#include <omp.h>
#include <cmath>
#include <iostream>
#include <utility>

namespace Planeverb
{
	// allocate memory for analysis results
	Analyzer::Analyzer(Grid * grid, FreeGrid* freeGrid) :
		m_grid(grid), m_freeGrid(freeGrid), m_results(nullptr)
	{
		// set up data
		vec2 gridSize = m_grid->GetGridSize();
		m_gridX = (unsigned)gridSize.x;
		m_gridY = (unsigned)gridSize.y; 
		m_responseLength = m_grid->GetResponseSize();
		m_samplingRate = m_grid->GetSamplingRate();
		m_dx = grid->GetDX();
		m_numThreads = grid->GetMaxThreads();
		m_resolution = grid->GetResolution();

		// find size for both grids, allocate pool of memory
		unsigned size =
			m_gridX * m_gridY * sizeof(AnalyzerResult) +
			m_gridX * m_gridY * sizeof(Real);
		m_mem = new char[size];
		if (!m_mem)
		{
			throw pv_NotEnoughMemory;
		}

		// set grid ptrs into pool
		m_results = reinterpret_cast<AnalyzerResult*>(m_mem);
		m_delaySamples = reinterpret_cast<Real*>(m_mem + m_gridX * m_gridY * sizeof(AnalyzerResult));
	}
	Analyzer::~Analyzer()
	{
		// delete pool of memory
		delete[] m_mem;
	}

	// analyze in parallel
	void Analyzer::AnalyzeResponses(const vec3& listenerPosGiven)
	{
		vec2 dim((Real)m_gridX, (Real)m_gridY);

		// set OMP thread count
		if (m_numThreads == 0)
			omp_set_num_threads(omp_get_max_threads());
		else
			omp_set_num_threads(m_numThreads);

		const int NUM_TASKS = 4;
		int gridSize = (int)m_gridX * (int)m_gridY;
		int gridIterations = gridSize * NUM_TASKS;
		vec3 listenerPos = listenerPosGiven;
		listenerPos.x += m_grid->GetGridOffset().x;
		listenerPos.z += m_grid->GetGridOffset().y;

		// reset delay values
		Real* delayLooper = m_delaySamples;
		Real maxVal = (Real)std::numeric_limits<Real>::max();
		for (int i = 0; i < gridSize; ++i)
			*delayLooper++ = maxVal;

		// each type of analysis can be done in parallel
		// each index can be done in parallel
		#pragma omp parallel for
		for (int i = 0; i < gridIterations; ++i)
		{
			// extract analysis type and real index from i
			unsigned analysisType = i % NUM_TASKS;
			unsigned index = i / NUM_TASKS;

			// convert index to grid position, to retrieve IR
			vec2 gridIndex;
			unsigned gridX, gridY;
			INDEX_TO_POS(gridX, gridY, index, dim);
			gridIndex.x = (Real)gridX;
			gridIndex.y = (Real)gridY;
			const Cell* response = m_grid->GetResponse(gridIndex);

			// use IR to analyze
			switch (analysisType)
			{
				case 0:	// analyze for occlusion
					m_results[index].occlusion = 
						AnalyzeOcclusion(index, response, listenerPos, m_responseLength);
					break;
				case 1:	// analyze for lowpass
					m_results[index].lowpassIntensity =
						AnalyzeLowpassIntensity(index, response, listenerPos, m_responseLength);
					break;
				case 2:	// analyze for decay time
					m_results[index].rt60 =
						AnalyzeDecayTime(index, response, m_responseLength);
					break;
				case 3:	// analyze for delay
					m_delaySamples[index] = AnalyzeDelay(index, response, m_responseLength);
					break;

			}
		}

		// run a post processing step to find directions based off of delays
		// can be run in parallel for each grid position
		#pragma omp parallel for
		for (int i = 0; i < gridSize; ++i)
		{
			// retrieve IR
			vec2 gridIndex;
			unsigned gridX, gridY;
			INDEX_TO_POS(gridX, gridY, i, dim);
			gridIndex.x = (Real)gridX;
			gridIndex.y = (Real)gridY;
			const Cell* response = m_grid->GetResponse(gridIndex);

			// analyze for direction
			m_results[i].direction = AnalyzeDirection(i, response, listenerPos, m_responseLength);
		}
	}

	const AnalyzerResult * Analyzer::GetResponseResult(const vec3 & emitterPos) const 
	{
		// retrieve analyzer result based off of an emitter position in world space
		const auto& offset = m_grid->GetGridOffset();
		unsigned posX = (unsigned)((emitterPos.x + offset.x) / m_dx); //(unsigned)(emitterPos.x + offset.x);
		unsigned posY = (unsigned)((emitterPos.z + offset.y) / m_dx); //(unsigned)(emitterPos.z + offset.y);
		if (posX > m_gridX || posY > m_gridY)
			return nullptr;
		const auto* res = &(m_results[INDEX(posX, posY, vec2((Real)m_gridX, (Real)m_gridY))]);
		return res;
	}

	Real Analyzer::AnalyzeOcclusion(unsigned index, const Cell * response, const vec3& listenerPos, unsigned numSamples)
	{
		// find emitter and listener grid positions
		int listenerX = (int)(listenerPos.x * (1.f / m_dx));
		int listenerY = (int)(listenerPos.z * (1.f / m_dx));
		vec2 dim((Real)m_gridX, (Real)m_gridY);
		int emitterX, emitterY;
		INDEX_TO_POS(emitterX, emitterY, index, dim);

		Real Edry = 0;
		bool found = false;
		int j = 0;
		int numIterations = (int)numSamples;
		// loop until the first audible sample, then continue for DRY_GAIN_ANALYSIS_LENGTH more seconds
		// sum signal values squared
		for (; j < numIterations; ++j)
		{
			Real next = response[j].pr;
			if (!found && std::abs(next) > PV_AUDIBLE_THRESHOLD_GAIN)
			{
				numIterations = (int)(PV_DRY_GAIN_ANALYSIS_LENGTH * (Real)m_samplingRate) + j;
				if (numIterations > (int)m_responseLength)
					numIterations = (int)m_responseLength;
				found = true;
			}
			else
			{
				Edry += next * next;
			}
		}

		// retrieve free energy, and divide by Euclidean distance between them
		Real EfreePr = m_freeGrid->GetEFreePerR(listenerX, listenerY, emitterX, emitterY);

		// find output gain
		Real E = (Edry / EfreePr);
		Real g = std::sqrt(E);
		
		return g;	
	}

	Real Analyzer::AnalyzeLowpassIntensity(unsigned index, const Cell* response, const vec3& listenerPos, unsigned numSamples)
	{
		// get input distance driven by inverse of occlusion
		Real r = AnalyzeOcclusion(index, response, listenerPos, numSamples);
		if(r > 0)
			r = Real(1.0) / r;

		// feed into equation
		// y = -147 + (18390) / (1 + (x / 12)^0.8 )
		Real cutoff = (Real)-147.f + ((Real)18390.f) / ((Real)1.f + std::pow(r / (Real)12.f, (Real)0.8f));

		return cutoff;
	}

	Real Analyzer::AnalyzeDecayTime(unsigned index, const Cell * response, unsigned numSamples)
	{
		// Version 2
		{
			// FIND THE T60 OF A SIGNAL
			//==========================
			//
			// Use backwards Schroeder integration
			//         ^ inf
			// I(t) = | (P(t))^2 dt
			//       v t
			//
			// For each point in the signal starting at the end, going backwards until the end of the impulse
			// the intensity at the point is sum of the signal squared.
			// Effectively: 
			//	s[i], i = 0...N-1 is the signal
			//	EnergyDecayCurve[i] = sum(s[i...N-1]^2)
			//	EnergyDecayCurveDB[i] = 10*log10(EnergyDecayCurve[i])
			// Taking the slope of the generated curve will give the T60
			// slope of I(t), given f = max seconds, the slope is the simple linear regression, as above:
			//
			// B = sum((x_i - xbar) * (y_i - ybar), 1, n)
			//	   ----------------------------------------
			//		     sum( (x_i - xbar)^2, 1, n )
			// i = index
			// x_i = t
			// xbar = average of x_i
			// y_i = EnergyDecayCurveDB[i]
			// ybar = average of y_i
			// 
			// To find the T60: 
			// T60 = -60dB / B

			// find starting point
			int startingPoint = 0;
			Real next = 0.f;
			Real samplingRate = (Real)m_samplingRate;
			int numIterations = (int)numSamples;
			for (int i = 0; i < numIterations; ++i)
			{
				next = response[i].pr;
				if (std::abs(next) > PV_AUDIBLE_THRESHOLD_GAIN)
				{
					startingPoint = i + (int)(PV_DRY_GAIN_ANALYSIS_LENGTH * samplingRate);
					break;
				}
			}

			// perform backwards integration
			// set up variables
			int end = numIterations - 1;
			int count = 0;
			Real invCount = 1.f;
			Real nextSquared = 0.f; 
			Real energyDecayCurve = 0.f;
			Real energyDecayCurveDB = 0.f;
			Real B = 0.f;
			Real x_i = 0.f, xbar = 0.f, xterm = 0.f;
			Real y_i = 0.f, ybar = 0.f, yterm = 0.f;
			Real numer = 0.f, denom = 0.f;

			// end point for when to start keeping track for linear regression
			int endPoint = end - (int)(PV_SCHROEDER_OFFSET_S * samplingRate);

			// loop backwards for Schroeder integration
			for (int i = end; i >= startingPoint; --i)
			{
				// calculate next schroeder value
				nextSquared = response[i].pr;
				energyDecayCurve += nextSquared * nextSquared;

				// calculate next schroeder value in dB
				energyDecayCurveDB = 10.f * std::log10(energyDecayCurve);

				// regression math
				if (i < endPoint)
				{
					++count;
					invCount = 1.f / (Real)count;

					// calculate x
					x_i = (Real)i / samplingRate;
					xbar += x_i;
					xterm = x_i - xbar * invCount;

					// calculate y
					y_i = energyDecayCurveDB;
					ybar += y_i;
					yterm = (y_i - ybar * invCount);

					// calculate numer
					numer += (xterm * yterm);

					// calculate denom
					denom += xterm * xterm;
				}
			}

			B = numer / denom;
			Real T60 = -60.f / B;
			return T60;
		}
	}

	Real Analyzer::AnalyzeDelay(unsigned index, const Cell * response, unsigned numSamples)
	{
		// find the first audible sample and return it's index
		int j = 0;
		int numIterations = (int)numSamples;
		for (; j < numIterations; ++j)
		{
			Real next = response[j].pr;
			if (std::abs(next) > PV_AUDIBLE_THRESHOLD_GAIN)
			{
				return (Real)j;
			}
		}

		// case no audible samples, return infinity
		return std::numeric_limits<Real>::max();
	}

	namespace
	{
		static const std::pair<int, int> POSSIBLE_NEIGHBORS[] = 
		{
			{-1, -1},	{-1, 0},	{-1, 1},
			{0, -1},				{0, 1},
			{1, -1},	{1, 0},		{1, 1}
		};
	} // namespace <>

	vec2 Analyzer::AnalyzeDirection(unsigned index, const Cell * response, const vec3& listenerPos, unsigned numSamples)
	{
		/* VERSION 2 */
		{
			Real loudness = m_results[index].occlusion;
			vec2 dim((Real)m_gridX, (Real)m_gridY);
			int nextIndex = index;
			const constexpr Real maxDelay = std::numeric_limits<Real>::max();
			Real nextDelay = maxDelay;
			Real samplingRate = (Real)m_samplingRate;
			Real wavelength = PV_C / (Real)m_resolution;
			const Real threshold = (Real)0.3f;
			Real thresholdDist = threshold * wavelength;

			// loop while not close to the listener
			while (nextDelay > PV_DELAY_CLOSE_THRESHOLD && loudness < PV_DISTANCE_GAIN_THRESHOLD)
			{
				int r, c;
				INDEX_TO_POS(r, c, nextIndex, dim);
				Real nextLoudness = 0.f;
				nextDelay = maxDelay;

				// for each neighbor find the neighbor with the smallest delay time
				for (int i = 0; i < _countof(POSSIBLE_NEIGHBORS); ++i)
				{
					int nr = r + POSSIBLE_NEIGHBORS[i].first;
					int nc = c + POSSIBLE_NEIGHBORS[i].second;
					if (nr < 0 || nc < 0 || nr >= (int)dim.x || nc >= (int)dim.y)
						continue;

					int newPosIndex = INDEX(nr, nc, dim);
					auto& result = m_results[newPosIndex];
					Real delay = m_delaySamples[newPosIndex];
					if (delay < nextDelay && result.occlusion > 0.f)
					{
						nextLoudness = result.occlusion;
						nextIndex = newPosIndex;
						nextDelay = delay;
					}
				}

				loudness = nextLoudness;

				// case couldn't find a valid neighbor
				if (nextDelay == std::numeric_limits<Real>::max())
				{
					break;
				}

				// line of sight check
				Real geodesicDist = PV_C * nextDelay / samplingRate;
				int r2, c2;
				INDEX_TO_POS(r2, c2, nextIndex, dim);

				// convert grid position to worldspace
				Real ex = (Real)r2 * m_dx;
				Real ey = (Real)c2 * m_dx;

				// find vector between and normalize
				vec2 temp(ex - listenerPos.x, ey - listenerPos.z);
				Real euclideanDist = (temp.x * temp.x) + (temp.y * temp.y);;
				euclideanDist = std::sqrt(euclideanDist);
				Real distCheck = std::abs(geodesicDist - euclideanDist);
				bool isInLineOfSight = distCheck < thresholdDist;
				if (isInLineOfSight)
					break;
			}

			// find direction vector between nextIndex and listener position

			// convert 1D index to 2D grid position
			int r, c;
			INDEX_TO_POS(r, c, nextIndex, dim);

			// convert grid position to worldspace
			Real ex = (Real)r * m_dx;
			Real ey = (Real)c * m_dx;

			// find vector between and normalize
			vec2 output(ex - listenerPos.x, ey - listenerPos.z);
			Real length = (output.x * output.x) + (output.y * output.y);
			if (length != 0.f)
			{
				length = std::sqrt(length);
				output.x /= length;
				output.y /= length;
			}

			return output;
		}
	}
} // namespace Planeverb
