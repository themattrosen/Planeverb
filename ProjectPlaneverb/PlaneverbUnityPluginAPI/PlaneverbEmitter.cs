using UnityEngine;

namespace Planeverb
{
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

		// assume each emitter can only emit one sound at a time for simplicity
		// this isn't meant to be an audio engine demonstration
		private int id = -1;
		private PlaneverbOutput output = new PlaneverbOutput();
		private PlaneverbAudioSource source = null;

		public int GetID() { return id; }
		public PlaneverbOutput GetOutput() { return output; }

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
			if (id >= 0)
			{ 
				// case this object is currently playing: update the emission, and update the output
				if(source)
				{
					PlaneverbContext.UpdateEmission(id, transform.position);
					output = PlaneverbContext.GetOutput(id);
				}
				// case this emission has ended since the last frame: end emission and reset the id
				else
				{
					OnEndEmission();
					id = -1;
				}
			}
		}

		void OnDestroy()
		{
			// case currently playing, end the emission in Planeverb
			// after Destroy, any PlaneverbAudioSource objects will detect it and destroy themselves
			if (id >= 0)
			{
				OnEndEmission();
			}
		}


		// two versions of Emit. one for playing stored Clip, other for AudioSource.PlayOneShot functionality

		public void Emit()
		{
			// start the emission and create the source
			id = PlaneverbContext.Emit(transform.position);
			source = PlaneverbAudioManager.pvDSPAudioManager.Play(Clip, id, this, Loop);
			if(source == null)
			{
				OnEndEmission();
			}
		}

		public void Emit(AudioClip clipToPlay)
		{
			// start the emission and create the source
			id = PlaneverbContext.Emit(transform.position);
			source = PlaneverbAudioManager.pvDSPAudioManager.Play(clipToPlay, id, this, Loop);
			if (source == null)
			{
				OnEndEmission();
			}
		}

		public void OnEndEmission()
		{
			// end the emission in planeverb, and reset ID
			PlaneverbContext.EndEmission(id);
			id = -1;
		}

		public float GetVolumeGain()
		{
			return volumeGain;
		}
	}
} // namespace Planeverb
