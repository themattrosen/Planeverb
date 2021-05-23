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

	// Add an emitter to the DSP scene
	PV_DSP_API EmissionID AddEmitter(float posX, float posY, float posZ,
		float forwardX, float forwardY, float forwardZ,
		float upX, float upY, float upZ);

	// Update an existing emitter in the DSP scene
	PV_DSP_API void UpdateEmitter(EmissionID id, float posX, float posY, float posZ,
		float forwardX, float forwardY, float forwardZ,
		float upX, float upY, float upZ);

	// Remove an emitter from the DSP scene
	PV_DSP_API void RemoveEmitter(EmissionID id);

	// Set the directivity pattern for an emitter
	PV_DSP_API void SetEmitterDirectivityPattern(EmissionID id, PlaneverbDSPSourceDirectivityPattern pattern);

	// Updates listener transform, needs world position and forward vector
	PV_DSP_API void SetListenerTransform(float posX, float posY, float posZ,
		float forwardX, float forwardY, float forwardZ,
		float upX, float upY, float upZ);

	// Submit audio source buffer for processing
	// @param id Emitter ID of the object sending source
	// @param dspParams Input parameters copied over from Planeverb output.
	// @param in Interleaved audio samples.
	// @param numFrames Number of FRAMES to use. One frame is 2 samples (stereo channels).
	PV_DSP_API void SendSource(EmissionID id, const PlaneverbDSPInput* dspParams, 
		const float* in, unsigned numFrames);

	// Retrieve pre-processed output buffers
	// @param dryOut gives the dry output buffer
	// @param outA gives an output buffer that feeds in to a reverb with 0.5s decay time
	// @param outB gives an output buffer that feeds in to a reverb with 1.0s decay time
	// @param outC gives an output buffer that feeds in to a reverb with 3.0s decay time
	PV_DSP_API void GetOutput(float** dryOut, float** outA, float** outB, float** outC);

} // namespace PlaneverbDSP
