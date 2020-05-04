using UnityEngine;

namespace Planeverb
{
	[RequireComponent(typeof(AudioListener))]
	[AddComponentMenu("Planeverb/PlaneverbListener")]
	public class PlaneverbListener : MonoBehaviour
	{
		void Start()
		{
			// init listener information in both contexts
			PlaneverbContext.SetListenerPosition(transform.position);
			PlaneverbDSPContext.SetListenerTransform(transform.position, transform.forward);
		}

		void Update()
		{
			// update listener information in both contexts
			PlaneverbContext.SetListenerPosition(transform.position);
			PlaneverbDSPContext.SetListenerTransform(transform.position, transform.forward);
		}
	}
} // namespace Planeverb
