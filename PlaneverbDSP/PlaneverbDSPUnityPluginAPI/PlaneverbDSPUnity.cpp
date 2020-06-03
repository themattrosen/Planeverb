#include "IUnityInterface.h"
#include "PlaneverbDSP.h"

#include <cstring>
#include <cmath>

#define PVU_EXPORT UNITY_INTERFACE_EXPORT
#define PVU_CC UNITY_INTERFACE_API

static float* g_buffA = nullptr;
static float* g_buffB = nullptr;
static float* g_buffC = nullptr;

namespace
{
	struct PlaneverbDSPInput
	{
		float obstructionGain;
		float rt60;
		float lowpass;
		float directionX;
		float directionY;
		float sourceDirectionX;
		float sourceDirectionY;
	};
}

extern "C"
{
#pragma region UnityPluginInterface
	PVU_EXPORT void PVU_CC 
	UnityPluginLoad(IUnityInterfaces* unityInterfaces){ (void)unityInterfaces; }

	PVU_EXPORT void PVU_CC
	UnityPluginUnload(){}
#pragma endregion

#pragma region Export Functions
	PVU_EXPORT void PVU_CC
	PlaneverbDSPInit(int maxCallbackLength, int samplingRate,
		int dspSmoothingFactor, bool useSpatialization)
	{
		PlaneverbDSP::PlaneverbDSPConfig config;
		config.maxCallbackLength = maxCallbackLength;
		config.samplingRate = samplingRate;
		config.dspSmoothingFactor = dspSmoothingFactor;
		config.useSpatialization = useSpatialization;

		PlaneverbDSP::Init(&config);
	}

	PVU_EXPORT void PVU_CC
	PlaneverbDSPExit()
	{
		PlaneverbDSP::Exit();
	}

	PVU_EXPORT void PVU_CC
	PlaneverbDSPSetListenerTransform(float posX, float posY, float posZ,
		float forwardX, float forwardY, float forwardZ)
	{
		PlaneverbDSP::SetListenerTransform(posX, posY, posZ, 
			forwardX, forwardY, forwardZ);
	}

	PVU_EXPORT void PVU_CC
	PlaneverbDSPUpdateEmitter(int emissionID, float forwardX, float forwardY, float forwardZ)
	{
		PlaneverbDSP::UpdateEmitter((PlaneverbDSP::EmissionID)emissionID, forwardX, forwardY, forwardZ);
	}

	PVU_EXPORT void PVU_CC
	PlaneverbDSPSetEmitterDirectivityPattern(int emissionId, int pattern)
	{
		PlaneverbDSP::SetEmitterDirectivityPattern((PlaneverbDSP::EmissionID)emissionId, 
			(PlaneverbDSP::PlaneverbDSPSourceDirectivityPattern)pattern);
	}

	PVU_EXPORT void PVU_CC
	PlaneverbDSPSendSource(int emissionID, const PlaneverbDSPInput* dspParams,
		const float* in, int numFrames)
	{
		PlaneverbDSP::PlaneverbDSPInput input;
		input.obstructionGain = dspParams->obstructionGain;
		input.lowpass = dspParams->lowpass;
		input.rt60 = dspParams->rt60;
		input.direction.x = dspParams->directionX;
		input.direction.y = dspParams->directionY;
		input.sourceDirectivity.x = dspParams->sourceDirectionX;
		input.sourceDirectivity.y = dspParams->sourceDirectionY;

		PlaneverbDSP::SendSource((PlaneverbDSP::EmissionID)emissionID,
			&input, in, (unsigned)numFrames);
	}

	PVU_EXPORT bool PVU_CC 
	PlaneverbDSPProcessOutput()
	{
		PlaneverbDSP::GetOutput(&g_buffA, &g_buffB, &g_buffC);
		if (!g_buffA) return false;
		else if (std::isnan(*g_buffA)) return false;

		return true;
	}

	PVU_EXPORT void PVU_CC 
	PlaneverbDSPGetBufferA(float** buf)
	{
		*buf = g_buffA;
	}
	
	PVU_EXPORT void PVU_CC 
	PlaneverbDSPGetBufferB(float** buf)
	{
		*buf = g_buffB;
	}
	
	PVU_EXPORT void PVU_CC 
	PlaneverbDSPGetBufferC(float** buf)
	{
		*buf = g_buffC;
	}

#pragma endregion
} // extern "C"
