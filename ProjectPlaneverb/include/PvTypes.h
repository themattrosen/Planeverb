#pragma once

#include "PvMathTypes.h"

namespace Planeverb
{
	enum PlaneverbErrorCode
	{
		pv_NotEnoughMemory, // a call to operator new failed
		pv_InvalidConfig,	// user passed Planeverb::Init() a config ptr with invalid data or an invalid config ptr
	};

	enum PlaneverbExecutionType
	{
		pv_CPU, // use CPU OpenMP threading
		//pv_GPU, // use GPU OpenCL threading - !!! Not supported !!!
	};

	// Resolution represents max representable frequency in the grid
	// Affects grid processing speed by changing the size per grid cell
	// Low or mid resolution should be used for 4 or less cores.
	enum PlaneverbResolution
	{
		pv_LowResolution = 275,
		pv_MidResolution = 375,
		pv_HighResolution = 500,
		pv_ExtremeResolution = 750,

		pv_DefaultResolution = pv_MidResolution
	};

	enum PlaneverbBoundaryType
	{
		pv_AbsorbingBoundary,	// walls of the grid absorb acoustic energy
		pv_ReflectingBoundary,	// walls of the grid reflect acoustic energy - !!! Not supported !!!
	};

	struct PlaneverbConfig
	{
		// grid size in meters
		vec2 gridSizeInMeters = {10.f, 10.f};

		// max frequency represented by the FDTD grid
		// simplified into different resolution bins
		int gridResolution = pv_DefaultResolution;

		// boundary type
		PlaneverbBoundaryType gridBoundaryType = pv_AbsorbingBoundary;

		// directory for Planeverb to store temporary files
		// must be set manually by user
		const char* tempFileDirectory;

		// thread usage
		unsigned maxThreadUsage = 0; // can specify number of threads, 0 means as many as possible, minimum 2 otherwise
		PlaneverbExecutionType threadExecutionType = pv_CPU; // CPU or GPU

		// grid world offset - !!! Not supported !!!
		vec2 gridWorldOffset = { 0.f, 0.f };
	};

	// Final acoustic output for an emitter
	struct PlaneverbOutput
	{
		float occlusion;
        float wetGain;
		float rt60;
		float lowpass;
		vec2 direction;
		vec2 sourceDirectivity;
	};

	// ID typedefs
	using EmissionID = size_t;
	using PlaneObjectID = size_t;

	// Planeverb external constants
	const constexpr PlaneObjectID PV_INVALID_PLANE_OBJECT_ID = (PlaneObjectID)(-1);
	const constexpr EmissionID PV_INVALID_EMISSION_ID = (EmissionID)(-1);
	const constexpr Real PV_INVALID_DRY_GAIN = (Real)-1.f;

	// Internal constants
	const constexpr Real PV_PI = (Real)3.141593f;						// PI
	const constexpr Real PV_RHO = (Real)1.2041f;						// air density
	const constexpr Real PV_C = (Real)343.21f;							// speed of sound
	const constexpr Real PV_Z_AIR = PV_RHO * PV_C;						// natural impedance of air
	const constexpr Real PV_INV_Z_AIR = (Real)1.f / PV_Z_AIR;			// inverse impedance for absorbing boundaries
	const constexpr Real PV_INV_Z_REFLECT = (Real)0.0f;					// inverse impedance for reflecting boundaries
	const constexpr Real PV_AUDIBLE_THRESHOLD_GAIN = (Real)0.00000316f;	// precalculated -110 dB converted to linear gain
    const constexpr Real PV_DRY_DIRECTION_ANALYSIS_LENGTH = (Real)0.005f;// length of time flux of first wavefront (source direction)
    const constexpr Real PV_DRY_GAIN_ANALYSIS_LENGTH = (Real)0.01f;		// length of time to process the initial pulse for occlusion
	const constexpr Real PV_WET_GAIN_ANALYSIS_LENGTH = (Real)0.080f;	// length of time to process early reflections
	const constexpr Real PV_SQRT_2 = (Real)1.4142136f;					// precalculated sqrt(2)
	const constexpr Real PV_SQRT_3 = (Real)1.7320508f;					// precalculated sqrt(3)
	const constexpr Real PV_MAX_AUDIBLE_FREQ = (Real)20000.f;			// maximum audible frequency for humans
	const constexpr Real PV_MIN_AUDIBLE_FREQ = (Real)20.f;				// minimum audible frequency for humans
	const constexpr Real PV_POINTS_PER_WAVELENGTH = (Real)4.f;			// number of cells per wavelength
	const constexpr Real PV_SCHROEDER_OFFSET_S = (Real)0.01f;			// experimentally calculated amount to cut off schroeder tail
	const constexpr Real PV_DISTANCE_GAIN_THRESHOLD = (Real)0.891251f;	// -1dB converted to linear gain
	const constexpr Real PV_DELAY_CLOSE_THRESHOLD = (Real)5.f;			// "close enough" delay threshold when analyzing for direction
	const constexpr Real PV_IMPULSE_RESPONSE_S = PV_SQRT_2 * Real(12.5) / PV_C + Real(0.15);			// number of seconds to collect per impulse response
	//                                                             ^ should be half of the scene width

	// struct to represent grid cells
	// 16 bytes
	struct Cell
	{
		Real pr;	// air pressure
		Real vx;	// x component of particle velocity
		Real vy;	// y component of particle velocity
		short b;	// B field packed into 2 2 byte fields
		short by;	// B field packed into 2 2 byte fields

		Cell(Real _pr = 0.f, Real _vx = 0.f, Real _vy = 0.f, int boundaryCoef = 1, int _by = 1) :
			pr(_pr),
			vx(_vx),
			vy(_vy),
			b((short)boundaryCoef),
			by((short)_by)
		{}
	};

} // Planeverb
