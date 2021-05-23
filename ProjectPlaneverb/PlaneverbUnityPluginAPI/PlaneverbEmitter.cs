using UnityEngine;

namespace Planeverb
{
	public enum PlaneverbSourceDirectivityPattern
	{
		Omni,
		Cardioid,

		Count
	}

	[AddComponentMenu("Planeverb/PlaneverbEmitter")]
	public class PlaneverbEmitter : MonoBehaviour
	{
		// public interface
		public AudioClip Clip;
		public bool PlayOnAwake;
		public bool Loop;

		[Range(-48f, 12f)]
		public float Volume;
		private float volumeGain;
		public PlaneverbSourceDirectivityPattern DirectivityPattern;

		// assume each emitter can only emit one sound at a time for simplicity
		// this isn't meant to be an audio engine demonstration
		private int pvID = -1;
		private int dspID = -1;
		private PlaneverbOutput output = new PlaneverbOutput();
		private PlaneverbAudioSource source = null;

		public int GetPlaneverbID() { return pvID; }
		public int GetDSPID() { return dspID; }

		public PlaneverbOutput GetOutput()
		{
			return output;
		}

		void Start()
		{
			if(PlayOnAwake)
			{
				Emit();
			}

			volumeGain = Mathf.Pow(10f, Volume / 20f);
		}

		void Update()
		{
			// case needs to update PV information
			if (pvID >= 0)
			{ 
				// case this object is currently playing: update the emission, and update the output
				if(source)
				{
					PlaneverbContext.UpdateEmitter(pvID, transform.position);
					PlaneverbDSPContext.UpdateEmitter(dspID, transform.position, transform.forward, transform.up);
					output = PlaneverbContext.GetOutput(pvID);
				}
				// case this emission has ended since the last frame: end emission and reset the id
				else
				{
					OnEndEmission();
					pvID = -1;
				}

				if(PlaneverbContext.GetInstance().debugDraw)
				{
					Debug.DrawRay(transform.position, transform.forward);
				}
			}
		}

		void OnDestroy()
		{
			// case currently playing, end the emission in Planeverb
			// after Destroy, any PlaneverbAudioSource objects will detect it and destroy themselves
			if (pvID >= 0)
			{
				OnEndEmission();
			}
		}


		// two versions of Emit. one for playing stored Clip, other for AudioSource.PlayOneShot functionality

		public void Emit()
		{
			// start the emission and create the source
			pvID = PlaneverbContext.AddEmitter(transform.position);
			dspID = PlaneverbDSPContext.AddEmitter(transform.position, transform.forward, transform.up);
			PlaneverbDSPContext.SetEmitterDirectivityPattern(dspID, DirectivityPattern);
			output = PlaneverbContext.GetOutput(pvID);
			source = PlaneverbAudioManager.pvDSPAudioManager.Play(Clip, dspID, this, Loop);
			if(source == null)
			{
				OnEndEmission();
			}
		}

		public void Emit(AudioClip clipToPlay)
		{
			// start the emission and create the source
			pvID = PlaneverbContext.AddEmitter(transform.position);
			dspID = PlaneverbDSPContext.AddEmitter(transform.position, transform.forward, transform.up);
			PlaneverbDSPContext.SetEmitterDirectivityPattern(dspID, DirectivityPattern);
			output = PlaneverbContext.GetOutput(pvID);
			source = PlaneverbAudioManager.pvDSPAudioManager.Play(clipToPlay, dspID, this, Loop);
			if (source == null)
			{
				OnEndEmission();
			}
		}

		public void OnEndEmission()
		{
			// end the emission in planeverb, and reset ID
			PlaneverbContext.RemoveEmitter(pvID);
			PlaneverbDSPContext.RemoveEmitter(dspID);
			pvID = -1;
		}

		public float GetVolumeGain()
		{
			return volumeGain;
		}
	}
} // namespace Planeverb
