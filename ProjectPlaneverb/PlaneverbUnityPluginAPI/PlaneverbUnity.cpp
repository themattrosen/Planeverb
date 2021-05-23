#pragma once
#include "IUnityInterface.h"
#include "Planeverb.h"

#define PVU_CC UNITY_INTERFACE_API
#define PVU_EXPORT UNITY_INTERFACE_EXPORT

extern "C"
{
#pragma region UnityPluginInterface

	void PVU_EXPORT PVU_CC UnityPluginLoad(IUnityInterfaces* unityInterfaces)
	{
		(void)unityInterfaces;
	}

	void PVU_EXPORT PVU_CC UnityPluginUnload()
	{

	}

#pragma endregion

#pragma region Export Functions
	PVU_EXPORT void PVU_CC
	PlaneverbInit(float gridSizeX, float gridSizeY,
		int gridResolution, int gridBoundaryType, char* tempFileDir, 
		int gridCenteringType,
		float gridWorldOffsetX, float gridWorldOffsetY
	)
	{
		Planeverb::PlaneverbConfig config;
		config.gridSizeInMeters.x = gridSizeX;
		config.gridSizeInMeters.y = gridSizeY;
		config.gridResolution = gridResolution;
		config.gridBoundaryType = (Planeverb::PlaneverbBoundaryType)gridBoundaryType;
		config.tempFileDirectory = tempFileDir;
		config.gridCenteringType = (Planeverb::PlaneverbGridCenteringType)gridCenteringType;
		config.gridWorldOffset.x = gridWorldOffsetX;
		config.gridWorldOffset.y = gridWorldOffsetY;

		Planeverb::Init(&config);
	}

	PVU_EXPORT void PVU_CC
	PlaneverbExit()
	{
		Planeverb::Exit();
	}

	PVU_EXPORT int PVU_CC
	PlaneverbAddEmitter(float x, float y, float z)
	{
		return (int)Planeverb::AddEmitter(Planeverb::vec3(x, y, z));
	}

	PVU_EXPORT void PVU_CC
	PlaneverbUpdateEmitter(int id, float x, float y, float z)
	{
		Planeverb::UpdateEmitter((Planeverb::EmissionID)id, Planeverb::vec3(x, y, z));
	}

	PVU_EXPORT void PVU_CC
	PlaneverbRemoveEmitter(int id)
	{
		Planeverb::RemoveEmitter((Planeverb::EmissionID)id);
	}

	// Output struct with only primitives
	struct PlaneverbOutput
	{
		float occlusion;
		float wetGain;
		float rt60;
		float lowpass;
		float directionX;
		float directionY;
		float sourceDirectionX;
		float sourceDirectionY;
	};

	PVU_EXPORT PlaneverbOutput PVU_CC
	PlaneverbGetOutput(int emissionID)
	{
		PlaneverbOutput output;
		auto poutput = Planeverb::GetOutput((Planeverb::EmissionID)emissionID);
		output.occlusion = poutput.occlusion;
		output.wetGain = poutput.wetGain;
		output.rt60 = poutput.rt60;
		output.lowpass = poutput.lowpass;
		output.directionX = poutput.direction.x;
		output.directionY = poutput.direction.y;
		output.sourceDirectionX = poutput.sourceDirectivity.x;
		output.sourceDirectionY = poutput.sourceDirectivity.y;
		return output;
	}

	PVU_EXPORT int PVU_CC
	PlaneverbAddGeometry(float posX, float posY,
		float width, float height, 
		float absorption)
	{
		Planeverb::AABB aabb;
		aabb.position.x = posX;
		aabb.position.y = posY;
		aabb.width = width;
		aabb.height = height;
		aabb.absorption = absorption;

		return (int)Planeverb::AddGeometry(&aabb);
	}

	PVU_EXPORT void PVU_CC
	PlaneverbUpdateGeometry(int id,
		float posX, float posY,
		float width, float height,
		float absorption)
	{
		Planeverb::AABB aabb;
		aabb.position.x = posX;
		aabb.position.y = posY;
		aabb.width = width;
		aabb.height = height;
		aabb.absorption = absorption;
		
		Planeverb::UpdateGeometry((Planeverb::PlaneObjectID)id, &aabb);
	}

	PVU_EXPORT void PVU_CC
	PlaneverbRemoveGeometry(int id)
	{
		Planeverb::RemoveGeometry((Planeverb::PlaneObjectID)id);
	}

	PVU_EXPORT void PVU_CC
	PlaneverbSetListenerPosition(float x, float y, float z)
	{
		Planeverb::SetListenerPosition(Planeverb::vec3(x, y, z));
	}
#pragma endregion

}
