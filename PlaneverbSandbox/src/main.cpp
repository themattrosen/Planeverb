//Matthew Rosen

#include "Graphics\Graphics.h"
#include "Editor\Editor.h"
#include "Audio\AudioCore.h"
#include <Planeverb.h>
#include <PlaneverbDSP.h>
#include <iostream>

int main()
{
	Graphics::Instance().Init();
	
	Planeverb::PlaneverbConfig config;
	config.gridResolution = Planeverb::pv_LowResolution;
	config.gridBoundaryType = Planeverb::pv_AbsorbingBoundary;
	config.gridSizeInMeters = Planeverb::vec2(10.f, 10.f);
	config.tempFileDirectory = ".";
	config.maxThreadUsage = 0;
	
	Planeverb::Init(&config);
	
	PlaneverbDSP::PlaneverbDSPConfig dspConfig;
	dspConfig.samplingRate = RATE;
	dspConfig.useSpatialization = true;
	dspConfig.dspSmoothingFactor = 5;
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
