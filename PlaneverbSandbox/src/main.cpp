//Matthew Rosen

#include "Graphics\Graphics.h"
#include "Editor\Editor.h"
#include "Audio\AudioCore.h"
#include <Planeverb.h>
#include <PlaneverbDSP.h>
#include <iostream>
#include <fstream>

ImVec2 LISTENING_REGION = { 20.f, 20.f };

int main()
{
	Graphics::Instance().Init();
	
	Planeverb::PlaneverbConfig config;
	config.gridResolution = Planeverb::pv_LowResolution;
	config.gridBoundaryType = Planeverb::pv_AbsorbingBoundary;
	config.gridCenteringType = Planeverb::pv_DynamicCentering;
	config.gridSizeInMeters = Planeverb::vec2(LISTENING_REGION.x, LISTENING_REGION.y);
	config.tempFileDirectory = ".";
	config.gridWorldOffset = { 0, 0 };
	
	Planeverb::Init(&config);
	
	PlaneverbDSP::PlaneverbDSPConfig dspConfig;
	dspConfig.samplingRate = RATE;
	dspConfig.useSpatialization = true;
	dspConfig.dspSmoothingFactor = 2;
	PlaneverbDSP::Init(&dspConfig);
	
	AudioCore::Instance().Init();
	Editor::Instance().Init(Graphics::Instance().GetWindow());
	
	while (Graphics::Instance().IsRunning())
	{
		Graphics::Instance().Update();
		Editor::Instance().Update();
		
		AudioCore::Instance().Update(Editor::Instance().GetEmitter());

		Graphics::Instance().Draw();
	}
	
	Editor::Instance().Exit();
	AudioCore::Instance().Exit();
	PlaneverbDSP::Exit();
	Planeverb::Exit();
	Graphics::Instance().Exit();

	return 0;
}
