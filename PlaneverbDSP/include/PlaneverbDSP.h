// Client include file that contains all externed functions
#pragma once

#include "PvDSPDefinitions.h"
#include "PvDSPTypes.h"

namespace PlaneverbDSP
{
	// Initialize the Planeverb DSP module
	PV_DSP_API void Init(const PlaneverbDSPConfig* config);

	// Shuts down the Planeverb DSP module
	PV_DSP_API void Exit();

	PV_DSP_API void UpdateEmitter(EmissionID id, float forwardX, float forwardY, float forwardZ);

	PV_DSP_API void SetEmitterDirectivityPattern(EmissionID id, PlaneverbDSPSourceDirectivityPattern pattern);

	// Updates listener transform, needs world position and forward vector
	PV_DSP_API void SetListenerTransform(float posX, float posY, float posZ,
		float forwardX, float forwardY, float forwardZ);

	// Submit audio source buffer for processing
	PV_DSP_API void SendSource(EmissionID id, const PlaneverbDSPInput* dspParams, 
		const float* in, unsigned numFrames);

	// Retrieve pre-processed output buffers
	// @param outA gives an output buffer that feeds in to a reverb with 0.5s decay time
	// @param outB gives an output buffer that feeds in to a reverb with 1.0s decay time
	// @param outC gives an output buffer that feeds in to a reverb with 3.0s decay time
	PV_DSP_API void GetOutput(float** outA, float** outB, float** outC);

} // namespace PlaneverbDSP
