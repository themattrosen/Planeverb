using UnityEngine;

namespace Planeverb
{
	[System.Serializable]
	public class PlaneverbDSPConfig
	{
		[Tooltip("Maximum length for the audio thread's callback, usually a power of 2.")]
		public int maxCallbackLength;
		
		[Tooltip("Factored into lerping dspParams over multiple audio callbacks.")]
		public int dspSmoothingFactor;

		[Tooltip("Unity's audio engine sampling rate. Usually 44100 or 48000.")]
		public int samplingRate;

		[Tooltip("Flag for whether PlaneverbDSP should process spatialization (true), or if the user will handle spatialization themselves (false).")]
		public bool useSpatialization;

		[Tooltip("Ratio for how much the reverberant sound affects the audio.")]
		public float wetGainRatio;
	}

} // namespace PlaneverbDSP