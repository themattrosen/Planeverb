// Client include file that contains all externed functions
#pragma once

#include "PvDefinitions.h"
#include "PvTypes.h"
#include <utility>

namespace Planeverb
{
	// Initialize the Planeverb acoustics module
	// Can throw pv_InvalidConfig or pv_NotEnoughMemory
	PV_API void Init(const PlaneverbConfig* config);
	
	// Shuts down the Planeverb acoustics module
	PV_API void Exit();

	// Very expensive call. 
	// Calls Exit and then Init again with the new config
	// Can throw pv_InvalidConfig or pv_NotEnoughMemory
	PV_API void ChangeSettings(const PlaneverbConfig* newConfig);

	// Begin tracking a new sound being played
	PV_API EmissionID AddEmitter(const vec3& emitterPosition);

	// Update information about a given emission
	PV_API void UpdateEmitter(EmissionID id, const vec3& position);

	// Stop tracking a sound that's finished playing
	PV_API void RemoveEmitter(EmissionID id);

	// Retrieve acoustic output for a given emitter
	PV_API PlaneverbOutput GetOutput(EmissionID emitter);

	// Checks if the output returned from GetOutput is a valid output.
	PV_API bool IsOutputValid(const PlaneverbOutput* output);

	// Add a new piece of geometry to the scene
	PV_API PlaneObjectID AddGeometry(const AABB* transform);

	// Update dynamic geometry in the scene
	PV_API void UpdateGeometry(PlaneObjectID id, const AABB* newTransform);

	// Removes dynamic geometry from the scene
	PV_API void RemoveGeometry(PlaneObjectID id);

	// Updates listener
	PV_API void SetListenerPosition(const vec3& listenerPosition);

	// Retrieves an Impulse Response for debugging purposes.
	PV_API std::pair<const Cell*, unsigned> GetImpulseResponse(const vec3& position);
	
} // namespace Planeverb
