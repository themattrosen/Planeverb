#include <DSP\Analyzer.h>
#include <FDTD\Grid.h>
#include <FDTD\FreeGrid.h>
#include <Context\PvContext.h>
#include <PvDefinitions.h>

#include <cmath>
#include <iostream>
#include <utility>
#include <algorithm>
#include <cassert>

namespace Planeverb
{
	// allocate memory for analysis results
	Analyzer::Analyzer(Grid * grid, FreeGrid* freeGrid, char* mem) :
		m_mem(mem),	m_grid(grid), m_freeGrid(freeGrid), m_results(nullptr)
	{
		// set up data
		const auto& globals = Context::GetGlobals();
		vec2 gridSize = globals.gridSize;
		unsigned gridX = (unsigned)gridSize.x;
		unsigned gridY = (unsigned)gridSize.y; 
		
		// find size for both grids, allocate pool of memory
		unsigned size =
			gridX * gridY * sizeof(AnalyzerResult) +
			gridX * gridY * sizeof(Real);
		if (!m_mem)
		{
			throw pv_NotEnoughMemory;
		}

		// set grid ptrs into pool
		m_results = reinterpret_cast<AnalyzerResult*>(m_mem);
		m_delaySamples = reinterpret_cast<Real*>(m_mem + gridX * gridY * sizeof(AnalyzerResult));
	}
	Analyzer::~Analyzer()
	{
		// memory owned by context, no deletion required
	}

	void Analyzer::AnalyzeResponses(const vec3& listenerPosGiven)
	{
		const auto& globals = Context::GetGlobals();
		vec2 dim = globals.gridSize;

        int gridSize = (int)dim.x * (int)dim.y;
		int responseLength = globals.responseSampleLength;

		vec3 listenerPos = listenerPosGiven;

		// reset delay values
		Real* delayLooper = m_delaySamples;
		Real maxVal = (Real)std::numeric_limits<Real>::max();
		for (int i = 0; i < gridSize; ++i)
			*delayLooper++ = maxVal;

		for (int serialIndex = 0; serialIndex < gridSize; ++serialIndex)
		{
			// convert index to grid position, to retrieve IR
			vec2 worldSpace;
			int col, row;
			INDEX_TO_POS(row, col, serialIndex, dim);
			m_grid->GridToWorld(col, row, worldSpace);
			const Cell* response = m_grid->GetResponse(worldSpace);

            EncodeResponse(serialIndex, worldSpace, response, listenerPos, responseLength);
		}

		// run a post processing step to find directions based off of delays
		for (int i = 0; i < gridSize; ++i)
		{
			// retrieve IR
			vec2 worldSpace;
			unsigned col, row;
			INDEX_TO_POS(row, col, i, dim);
			m_grid->GridToWorld(col, row, worldSpace);
			const Cell* response = m_grid->GetResponse(worldSpace);

			// analyze for listener direction
			m_results[i].direction = EncodeListenerDirection(i, response, listenerPos, responseLength);
		}
	}

	const AnalyzerResult * Analyzer::GetResponseResult(const vec3 & emitterPos) const 
	{
		// retrieve analyzer result based off of an emitter position in world space
		const auto& globals = Context::GetGlobals();
		int posX, posY;
		m_grid->WorldToGrid({ emitterPos.x, emitterPos.z }, posX, posY);
		if (posX > globals.gridSize.x || posY > globals.gridSize.y || posX < 0 || posY < 0)
			return nullptr;
		const auto* res = &(m_results[INDEX(posY, posX, globals.gridSize)]);
		return res;
	}

	unsigned Analyzer::GetMemoryRequirement(const PlaneverbConfig * config)
	{
		Real m_dx, m_dt;
		Real samplingRate;
		CalculateGridParameters(config->gridResolution, m_dx, m_dt, samplingRate);
		
		vec2 m_gridSize;
		m_gridSize.x = (1.f / m_dx) * config->gridSizeInMeters.x;
		m_gridSize.y = (1.f / m_dx) * config->gridSizeInMeters.y;

		unsigned m_gridX = (unsigned)m_gridSize.x;
		unsigned m_gridY = (unsigned)m_gridSize.y;
		
		// find size for both grids, allocate pool of memory
		unsigned size =
			m_gridX * m_gridY * sizeof(AnalyzerResult) +
			m_gridX * m_gridY * sizeof(Real);

		return size;
	}

    void Analyzer::EncodeResponse(unsigned serialIndex, vec2 worldSpace, const Cell* response, const vec3& listenerPos, unsigned n)
    {
        const int numSamples = (int)n;
		const auto& globals = Context::GetGlobals();
		Real samplingRate = globals.samplingRate;
		Real dx = globals.gridDX;

        // 
        // ONSET DELAY
        // 
        int onsetSample = 0;
        for (; onsetSample < numSamples; ++onsetSample)
        {
            Real next = response[onsetSample].pr;
            if (std::abs(next) > PV_AUDIBLE_THRESHOLD_GAIN)
            {
                break;
            }
        }

        if (onsetSample < numSamples)
        {
            m_delaySamples[serialIndex] = (Real)onsetSample;
        }
        //no onset found, fill infinity and bail, can't encode anything else.
        else
        {
            m_delaySamples[serialIndex] = std::numeric_limits<Real>::max();
            return;
        }

        // 
        // DRY PROCESSING: OBSTRUCTION and SOURCE DIRECTION
        //
        int directGainSamples = (int)(PV_DRY_GAIN_ANALYSIS_LENGTH * samplingRate);
        int sourceDirSamples = (int)(PV_DRY_DIRECTION_ANALYSIS_LENGTH * samplingRate);
        int sourceDirEnd = onsetSample + sourceDirSamples;
        int directEnd = onsetSample + directGainSamples;

        PV_ASSERT(sourceDirSamples <= directGainSamples && "Code below assumes source directivity is estimated on a shorter interval of time than dry gain.");

        Real obstructionGain = 0.0f;
        vec2 radiationDir(0,0);
        {
            Real Edry = 0;
			const Cell* responsePtr = response;

            int j = 0;
            for (; j < sourceDirEnd; ++j)
            {
                const auto& r = *responsePtr++;
                Edry += r.pr * r.pr;
                radiationDir.x += r.pr * r.vx;
                radiationDir.y += r.pr * r.vy;
            }

            for (; j < directEnd; ++j)
            {
                const auto& r = *responsePtr++;
                Edry += r.pr * r.pr;
            }

            // Normalize dry energy by free-space energy to obtain geometry-based 
            // obstruction gain with distance attenuation factored out
            Real EfreePr = m_freeGrid->GetEFreePerR(vec2(listenerPos.x, listenerPos.z), worldSpace);

            Real E = (Edry / EfreePr);
            obstructionGain = std::sqrt(E);

            // Normalize and negate flux direction to obtain radiated unit vector
            Real norm = radiationDir.Length();
            norm = Real(-1.0f) / (norm > Real(0.0f) ? norm : Real(1.0f));
            radiationDir.x *= norm;
            radiationDir.y *= norm;
        }
        
        m_results[serialIndex].occlusion = obstructionGain;
        m_results[serialIndex].sourceDirectivity = radiationDir;

        //
        // LOW-PASS CUTOFF FREQUENCY
        //

        // get input distance driven by inverse of occlusion. If occlusion is very small, cap out at "lots of occlusion"
        Real r = 1.0f / std::max(0.001f, obstructionGain);
        
        // Find LPF cutoff frequency by feeding into equation: y = -147 + (18390) / (1 + (x / 12)^0.8 )
        const Real powNumer = r / Real(12.f);
        const Real denom = Real(1.f) + std::pow(powNumer, Real(0.8f));
        const Real fraction = (Real(18390.f)) / (denom);
        m_results[serialIndex].lowpassIntensity = Real(-147.f) + fraction;

        //
        // Wet gain
        //
        Real wetEnergy = 0.0f;
        {
            const int wetGainSamples = (int)(PV_WET_GAIN_ANALYSIS_LENGTH * samplingRate);
            const int end = std::min(directEnd + 1 + wetGainSamples, numSamples);
            for (int j = directEnd + 1; j < end; j++)
            {
                const float p = response[j].pr;
                wetEnergy += p * p;
            }
        }

        // Normalize as if source had unit energy at 1m distance
        m_results[serialIndex].wetGain = std::sqrt(wetEnergy / m_freeGrid->GetEnergyAtOneMeter());

        //
        // Decay Time
        //
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

            int startingPoint = directEnd + 1;
            // linear regression ignores some fixed bit of tail of energy decay curve which dips towards 0
            int endPoint = numSamples - (int)(PV_SCHROEDER_OFFSET_S * samplingRate);
            int regressN = endPoint - startingPoint;
            Real rn = Real(regressN);

            // We regress assuming time-step is 1 and startingPoint is x=0.
            // The latter offset does not change slope, and time-step adjustment is done at end.
            Real xmean = (rn - 1.0f) * 0.5f;
            Real xsum = rn * xmean;
            
            // Sum[(x-xmean)^2] = Sum[(i - ((n - 1)/2))^2, {i, 0, n - 1}] = 1/12 n (-1 + n^2)
            Real denominator = (1.0f / 12.0f) * rn * (rn*rn - 1.0f);

            // Backward energy integral
            Real energyDecayCurve = 0.f;
            Real energyDecayCurveDB = 0.f;
            Real xysum = 0;
            Real ysum = 0;

            // For tail bit just accumulate energy, no regression
            for (int i = numSamples - 1; i >= endPoint; --i)
            {
                auto p = response[i].pr;
                energyDecayCurve += p * p;
            }

            for (int i = endPoint-1; i >= startingPoint; --i)
            {
                auto p = response[i].pr;
                energyDecayCurve += p * p;
                energyDecayCurveDB = 10.f * std::log10(energyDecayCurve);

                Real y_i = energyDecayCurveDB;
                auto x_i = (i - startingPoint);
                xysum += y_i * x_i;
                ysum += y_i;
            }

            Real ymean = ysum / rn;
            Real numerator = xysum - ymean * xsum - xmean * ysum + rn * xmean * ymean;
            
            Real slopeDBperSample = numerator / denominator;
            Real slopeDBperSec = slopeDBperSample * samplingRate;
            m_results[serialIndex].rt60 = -60.f / slopeDBperSec;
        }
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

    vec2 Analyzer::EncodeListenerDirection(unsigned index, const Cell * response, const vec3& listenerPos, unsigned numSamples)
    {
		const auto& globals = Context::GetGlobals();
        Real loudness = m_results[index].occlusion;
        vec2 dim = globals.gridSize;
        int nextIndex = index;
        const constexpr Real maxDelay = std::numeric_limits<Real>::max();
        Real delay = maxDelay;
        Real nextDelay = maxDelay;
        Real samplingRate = globals.samplingRate;
        Real wavelength = PV_C / Real(globals.config.gridResolution);
        const constexpr Real threshold = Real(0.3f);
        Real thresholdDist = threshold * wavelength;

        // loop while not close to the listener
        while (delay > PV_DELAY_CLOSE_THRESHOLD && loudness < PV_DISTANCE_GAIN_THRESHOLD)
        {
            int r, c;
            INDEX_TO_POS(r, c, nextIndex, dim);
            Real nextLoudness = 0.f;
            Real nextDelay = maxDelay;

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

                // check if sound reaches this index at all
                if ((unsigned)delay == numSamples || result.occlusion == 0.f)
                    continue;
                else if (delay < nextDelay && result.occlusion > 0.f)
                {
                    nextLoudness = result.occlusion;
                    nextIndex = newPosIndex;
                    nextDelay = delay;
                }
            }

            // case couldn't find a valid neighbor
            if (nextDelay == std::numeric_limits<Real>::max() || nextDelay >= delay)
            {
                break;
            }

            delay = nextDelay;
            loudness = nextLoudness;

            // line of sight check
            Real geodesicDist = PV_C * (nextDelay / samplingRate);
            int r2, c2;
            INDEX_TO_POS(r2, c2, nextIndex, dim);

            // convert grid position to worldspace
			vec2 emitterWorldSpace;
			m_grid->GridToWorld(c2, r2, emitterWorldSpace);

            // find vector between and check if within distance threshold
            Real euclideanDist = emitterWorldSpace.Distance(vec2(listenerPos.x, listenerPos.z));
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
		vec2 emitterWorldSpace;
		m_grid->GridToWorld(c, r, emitterWorldSpace);

        // find vector between and normalize
        vec2 output(emitterWorldSpace - vec2(listenerPos.x, listenerPos.z));
        output.Normalize();

        return output;
    }
} // namespace Planeverb
