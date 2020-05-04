using System;
using UnityEngine;

namespace Planeverb
{
	[AddComponentMenu("Planeverb/DSP/PlaneverbDSPAudioManager")]
	class PlaneverbAudioManager : MonoBehaviour
	{
		// global singleton
		public static PlaneverbAudioManager pvDSPAudioManager = null;

		// template to instantiate source objects
		private GameObject sourceTemplate = null;

		private void Awake()
		{
			// set singleton object
			pvDSPAudioManager = this;

			// create source template
			sourceTemplate = new GameObject("PlaneverbAudioSourceTemplate");
			sourceTemplate.AddComponent<PlaneverbAudioSource>();
			sourceTemplate.transform.SetParent(transform.parent);
		}

		public PlaneverbAudioSource Play(AudioClip clip, int id, PlaneverbEmitter emitter, bool loop)
		{
			if (clip)
			{
				// make a new source to be played next audio frame
				GameObject newSource = Instantiate(sourceTemplate);
				PlaneverbAudioSource newComp = newSource.GetComponent<PlaneverbAudioSource>();

				// set information on new object
				newComp.SetClip(clip);
				newComp.SetEmitter(emitter);
				newComp.SetManager(gameObject);
				newComp.SetLoop(loop);

				// add it as a child
				newSource.transform.SetParent(transform);

				return newComp;
			}
			else
			{
				return null;
			}
		}
	}
}
