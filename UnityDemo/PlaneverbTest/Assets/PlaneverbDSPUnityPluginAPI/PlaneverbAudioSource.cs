using System;
using UnityEngine;

namespace Planeverb
{
	[AddComponentMenu("Planeverb/DSP/PlaneverbAudioSourceInternal")]
	class PlaneverbAudioSource : MonoBehaviour
	{
		// handle to the clip that's being played
		private AudioClip clip = null;

		// name of the audio clip (for debugging purposes)
		private string clipName = null; 

		// read index into the clip data
		private int readIndex = 0;

		// playing flag
		private bool isPlaying = false;

		// looping flag
		private bool shouldLoop = false;

		// handle to the audio manager
		private GameObject audioManager = null;

		// handle to the emitter that spawned this source
		// used to pull acoustic data from
		private PlaneverbEmitter emitter;

		// acoustic data
		private PlaneverbDSPInput dspParams;

		// full buffer from the clip
		private float[] clipData = null;

		// buffer that's used during the audio callback
		private float[] runtimeArray = new float[PlaneverbDSPContext.MAX_FRAME_LENGTH];

		// total number of samples in the clip
		private int samples = 0;

		private void Start()
		{
			// initialize read index and 
			readIndex = 0;
			dspParams = new PlaneverbDSPInput();
		}

		private void Update()
		{
			// case this has stopped playing and the emitter still exists
			if (!isPlaying && emitter)
			{
				emitter.OnEndEmission();
				Destroy(gameObject);
			}
			// case this is playing, and the emitter doesn't exist
			else if(!emitter && isPlaying)
			{
				isPlaying = false;
				Destroy(gameObject);
			}
		}

		// playing getter
		public bool IsPlaying() { return isPlaying; }

		// set data to play
		public void SetClip(AudioClip newClip)
		{
			if (!newClip)
			{
				clip = null;
				readIndex = 0;
				clipName = null;
				samples = 0;
				clipData = null;
				isPlaying = false;
			}
			else
			{
				clip = newClip;
				clipName = clip.name;
				readIndex = 0;
				samples = clip.samples * clip.channels;
				clipData = new float[samples];
				bool valid = clip.GetData(clipData, 0);
				if(!valid)
				{
					Debug.Log("Oh?");
				}
				isPlaying = true;
			}
		}

		// set handle to the manager
		public void SetManager(GameObject man)
		{
			audioManager = man;
		}

		// set handle to the emitter this is playing from
		public void SetEmitter(PlaneverbEmitter e)
		{
			emitter = e;
		}

		// set loop flag
		public void SetLoop(bool loop)
		{
			shouldLoop = loop;
		}

		// called during audio thread, retrieves the next buffer of audio
		public float[] GetSource(int numSamples, int channels)
		{
			// only process frame if currently playing
			if (isPlaying)
			{
				// find the end index for the clipdata buffer
				int realSamplesEnd = Mathf.Min(readIndex + numSamples, samples);

				// find the real number of samples to use (in case this is the end of the clip)
				int realSamplesToUse = realSamplesEnd - readIndex;
				
				// memcpy the values over
				//Array.Copy(clipData, readIndex, runtimeArray, 0, realSamplesToUse);

				// apply volume
				float volume = emitter.GetVolumeGain();
				for(int i = 0, j = readIndex; i < realSamplesToUse; ++i, ++j)
				{
					runtimeArray[i] = clipData[j] * volume;
				}

				// increment the readindex into the clipdata
				readIndex += realSamplesToUse;

				// case the clip is finished playing
				if (realSamplesToUse < numSamples)
				{
					// case no looping, set playing flag to false to destroy this object
					if (!shouldLoop)
					{
						isPlaying = false;
					}
					// in case of looping
					else
					{
						// reset readindex
						readIndex = 0;

						// figure out the number of samples left to fill the data buffer
						int numSamplesLeft = numSamples - realSamplesToUse;

						// memcpy data over
						Array.Copy(clipData, readIndex, runtimeArray, realSamplesToUse, numSamplesLeft);

						// increment readIndex again
						readIndex += numSamplesLeft;
					}
				}
			}
			return runtimeArray;
		}

		// getters
		public int GetEmissionID() { return emitter.GetID(); }

		public PlaneverbDSPInput GetInput()
		{
			PlaneverbOutput pvoutput = emitter.GetOutput();
			dspParams.obstructionGain = pvoutput.occlusion;
			dspParams.wetGain = pvoutput.wetGain;
			dspParams.lowpass = pvoutput.lowpass;
			dspParams.rt60 = pvoutput.rt60;
			dspParams.directionX = pvoutput.directionX;
			dspParams.directionY = pvoutput.directionY;
			dspParams.sourceDirectionX = pvoutput.sourceDirectionX;
			dspParams.sourceDirectionY = pvoutput.sourceDirectionY;
			return dspParams;
		}
	}
}
