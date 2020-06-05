using System;

using UnityEngine;

namespace Planeverb
{
	[RequireComponent(typeof(AudioSource))]
	[AddComponentMenu("Planeverb/DSP/PlaneverbReverb")]
	class PlaneverbReverb : MonoBehaviour
	{
		// a value on the range [0, 3), represents the index into the output fetcher array
		public int myIndex = -1;
		public bool isDry = false;
		public static int MAX_REVERBS = 4;
		private static int runtimeIndex = 0;
		private static bool pvDSPProcessFlag = false;

		private PlaneverbAudioSource[] pvSources;
		private PlaneverbAudioSource[] audioThreadSources = new PlaneverbAudioSource[1024];
		private float[] outputBuffer = new float[PlaneverbDSPContext.MAX_FRAME_LENGTH];

		private void Awake()
		{
			// Dry     myIndex = 0
			// ReverbA myIndex = 1
			// ReverbB myIndex = 2
			// ReverbC myIndex = 3
			Debug.Assert(myIndex >= 0 && myIndex < MAX_REVERBS,
				"PlaneverbReverb MyIndex not set properly!");

			// ensure must be attached to an object with an AudioSource component
			Debug.Assert(GetComponent<AudioSource>() != null, 
				"PlaneverbReverb component attached to GameObject without an AudioSource!");
		}

		private void Update()
		{
			// update the sources array from the audio manager children list
			pvSources = 
				PlaneverbAudioManager.
				pvDSPAudioManager.
				GetComponentsInChildren<PlaneverbAudioSource>();
		}

		private void OnAudioFilterRead(float[] data, int channels)
		{
			int dataBufferLength = data.Length;

			// case: first reverb component to run during this audio frame, and there are emitters playing
			if(runtimeIndex == 0 && pvSources != null)
			{
				// copy over PVDSP Audio sources into threaded buffer
				int numSources = pvSources.Length;
				for (int i = 0; i < pvSources.Length; ++i)
				{
					audioThreadSources[i] = pvSources[i];
				}

				// get source buffer from each PVDSP Audio Source
				float[] buffer;
				for(int i = 0; i < numSources; ++i)
				{
					buffer = audioThreadSources[i].GetSource(dataBufferLength, channels);
					PlaneverbDSPContext.SendSource(
						pvSources[i].GetEmissionID(),
						pvSources[i].GetInput(), 
						buffer, dataBufferLength,
						channels);
				}

				// Tell the DSP Context to process the source and prepare into the 3 output buffers
				// ProcessOutput returns a flag on whether or not the output was successful
				pvDSPProcessFlag = PlaneverbDSPContext.ProcessOutput();
			}

			// increment the runtime index looping back around to zero from 3
			runtimeIndex = (runtimeIndex + 1) % MAX_REVERBS;

			// fill the in/out data buffer IFF output was processed successfully
			if (pvDSPProcessFlag == true)
			{
				// fetch the respective output buffer from the context
				// (each reverb has it's own output buffer based off of myIndex)
				PlaneverbDSPContext.GetOutputBuffer(myIndex, ref outputBuffer);
				
				// choose the right length in case data buffer too big
				dataBufferLength = (dataBufferLength > outputBuffer.Length) ? outputBuffer.Length : dataBufferLength;

				// memcpy the data over
				Array.Copy(outputBuffer, data, dataBufferLength);
			}
			// case that the PVDSP module couldn't generate valid output
			else
			{
				// fill output with 0
				for(int i = 0; i < dataBufferLength; ++i)
				{
					data[i] = 0f;
				}
			}
		}

	}
}
