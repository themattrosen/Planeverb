#pragma once

#include "PlaneverbDSP.h"

namespace PlaneverbDSP
{
	// Forward declares
	class EmissionsManager;
	
	// DSP context singleton 
	class Context
	{
	public:
		// init/exit context
		Context(const PlaneverbDSPConfig* config);
		~Context();

		// internal submit source buffer function
		void SubmitSource(EmissionID id, const PlaneverbDSPInput* dspParams,
			const float* in, unsigned numFrames);

		// retrieve output
		void GetOutput(float** outA, float** outB, float** outC);

		// update global listener transform
		void SetListenerTransform(const vec3& position, const vec3& forward);

		EmissionsManager* GetEmissionManager() { return m_emissions; }

	private:
		PlaneverbDSPConfig m_config;			// copy of the user configuration
		unsigned m_bufferSize;					// size in bytes of each buffer

		struct
		{
			vec3 position;
			vec3 forward;
			// assume up is (0, 1, 0)
		} m_listenerTransform;					// anonymous struct of listener information

		// memory for all buffers allocated at once
		// stored in m_mem
		char* m_mem = nullptr;

		// 4 input/output buffers 
		float* m_inputBufferStorage = nullptr;
		float* m_outputBufferA = nullptr;
		float* m_outputBufferB = nullptr;
		float* m_outputBufferC = nullptr;

		// double buffered 3 output buffers
		float* m_outputBufferA_1 = nullptr;
		float* m_outputBufferB_1 = nullptr;
		float* m_outputBufferC_1 = nullptr;
		float* m_outputBufferA_2 = nullptr;
		float* m_outputBufferB_2 = nullptr;
		float* m_outputBufferC_2 = nullptr;

		int m_numFrames = 0;				// number of frames of audio data sent in this audio callback

		// emissions handle
		EmissionsManager* m_emissions = nullptr;

		// test convolution ptrs, non-functional
		class ImpulseResponse* m_responseA;
		class Convolver* m_convolverA;
	};

	// Context singleton getter
	Context* GetContext();

} // namespace PlaneverbDSP
