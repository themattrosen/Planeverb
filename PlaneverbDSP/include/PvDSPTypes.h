#pragma once

namespace PlaneverbDSP
{
	// Internal constants
	const constexpr unsigned short PV_DSP_MAX_CALLBACK_LENGTH = 4096;
	const constexpr unsigned short PV_DSP_CHANNEL_COUNT = 2;
	const constexpr float PV_DSP_PI = 3.141593f;
	const constexpr float PV_DSP_SQRT_2 = 1.4142136f;
	const constexpr float PV_DSP_INV_SQRT_2 = 1.f / PV_DSP_SQRT_2;
	const constexpr float PV_DSP_MAX_AUDIBLE_FREQ = 20000.f;
	const constexpr float PV_DSP_MIN_AUDIBLE_FREQ = 20.f;
	const constexpr float PV_DSP_T_ER_1 = 0.5f;
	const constexpr float PV_DSP_T_ER_2 = 1.0f;
	const constexpr float PV_DSP_T_ER_3 = 3.0f;
	const constexpr float PV_DSP_MIN_DRY_GAIN = 0.01f;

	enum PlaneverbDSPErrorCode
	{
		pvd_NotEnoughMemory,	// a call to operator new failed
		pvd_InvalidConfig,		// user passed PlaneverbDSP::Init() a config ptr with invalid data or an invalid config ptr
	};

	enum PlaneverbDSPSourceDirectivityPattern
	{
		pvd_Omni,			// omni pattern
		pvd_Cardioid,		// cardioid 
		// add more here

		pvd_SourceDirectivityPatternCount
	};

	struct PlaneverbDSPConfig
	{
		// maximum size of the audio callback buffer in frames
		unsigned short maxCallbackLength = PV_DSP_MAX_CALLBACK_LENGTH;

		// factored into lerping dspParams over multiple audio callbacks
		// 1 means DSP parameters are lerped over only 1 audio callback
		// 5 means lerped over 5 separate audio callbacks
		// must be greater than 0
		unsigned short dspSmoothingFactor = 2;

		// sampling rate of audio engine
		// must be set manually by user
		unsigned samplingRate;

		// true -  use PlaneverbDSP built in spatialization engine (VBAP)
		// false - user will spatialize sound before sending buffers to PlaneverbDSP
		bool useSpatialization = true;

		float wetGainRatio = 0.9f;
	};

	struct vec2
	{
		float x, y;
		vec2(float _x = 0, float _y = 0) : x(_x), y(_y) {}
		inline float Dot(const vec2& rhs) const { return x * rhs.x + y * rhs.y; }
	};

	struct vec3
	{
		float x, y, z;
		vec3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
	};

	// Input information from the acoustics module
	struct PlaneverbDSPInput
	{
		float obstructionGain;
		float wetGain;
		float rt60;
		float lowpass;
		vec2 direction;
		vec2 sourceDirectivity;
	};

	// ID typedefs
	using EmissionID = size_t;
	const constexpr EmissionID PV_INVALID_EMISSION_ID = (EmissionID)(-1);
}
