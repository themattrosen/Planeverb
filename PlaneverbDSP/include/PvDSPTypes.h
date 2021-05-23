#pragma once
#include <cmath>

namespace PlaneverbDSP
{
	// Internal constants
	const constexpr unsigned short PV_DSP_MAX_CALLBACK_LENGTH = 4096;
	const constexpr unsigned short PV_DSP_CHANNEL_COUNT = 2;
	const constexpr unsigned PV_DSP_DEFAULT_MAX_EMITTERS = 64;
	const constexpr float PV_DSP_DEFAULT_WET_RATIO = 0.9f;
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
		unsigned short dspSmoothingFactor = 1;

		// sampling rate of audio engine
		// must be set manually by user
		unsigned samplingRate;

		// true -  use PlaneverbDSP built in spatialization engine (VBAP)
		// false - user will spatialize sound before sending buffers to PlaneverbDSP, this doesn't really work
		bool useSpatialization = true;

		// ratio between wet signal and dry signal
		float wetGainRatio = PV_DSP_DEFAULT_WET_RATIO;

		// maximum number of emitters allowed in the scene at the same time. 
		// includes emitters outside of the Planeverb grid.
		unsigned maxEmitters = PV_DSP_DEFAULT_MAX_EMITTERS;
	};

	struct vec2
	{
		float x, y;
		vec2(float _x = 0, float _y = 0) : x(_x), y(_y) {}
		vec2(const vec2&) = default;
		inline float Dot(const vec2& rhs) const { return x * rhs.x + y * rhs.y; }
		inline float SquareLength() const { return Dot(*this); }
		inline float Length() const { return std::sqrt(SquareLength()); }
		inline void Normalize() { float len(Length()); if (len > 0) { x /= len; y /= len; } }
		inline vec2 Normalized() const { vec2 ret(*this); ret.Normalize(); return ret; }
	};

	struct vec3
	{
		float x, y, z;
		vec3(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}
		vec3(const vec3& rhs) = default;
		vec3& operator=(const vec3& rhs) = default;
		~vec3() = default;
		vec3 operator-(const vec3& rhs) { return vec3(x - rhs.x, y - rhs.y, z - rhs.z); }
		vec3 operator+(const vec3& rhs) { return vec3(x + rhs.x, y + rhs.y, z + rhs.z); }
		vec3 operator*(float scalar) { return vec3(x * scalar, y * scalar, z * scalar); }

		inline float SquareLength() const { return x * x + y * y + z * z; }
		inline float Length() const { return std::sqrt(SquareLength()); }
		inline float SquareDistance(const vec3& rhs) { return vec3(*this - rhs).SquareLength(); }
		inline float Distance(const vec3& rhs) { return std::sqrt(SquareDistance(rhs)); }
		inline void Normalize() { float len(Length()); if (len > 0) { x /= len; y /= len; } }
		inline vec3 Normalized() const { vec3 ret(*this); ret.Normalize(); return ret; }
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
	const constexpr float PV_INVALID_OCCLUSION = -1.0f;
}
