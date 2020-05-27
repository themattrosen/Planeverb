using UnityEngine;

namespace Planeverb
{
	[RequireComponent(typeof(AudioListener))]
	[AddComponentMenu("Planeverb/PlaneverbListener")]
	public class PlaneverbListener : MonoBehaviour
	{
		private static PlaneverbListener instance = null;
		private Vector3 oldPosition;
		private const float CHANGE_EPSILON = 0.01f;

		void Start()
		{
			// enable singleton pattern
			Debug.AssertFormat(instance == null, "More than one instance of the PlaneverbListener created! Singleton violated.");
			instance = this;

			// init listener information in both contexts
			PlaneverbContext.SetListenerPosition(transform.position);
			PlaneverbDSPContext.SetListenerTransform(transform.position, transform.forward);
		}

		void Update()
		{
			// update listener information in both contexts
			PlaneverbContext.SetListenerPosition(transform.position);
			PlaneverbDSPContext.SetListenerTransform(transform.position, transform.forward);
			oldPosition = transform.position;

			if(PlaneverbContext.GetInstance().debugDraw)
			{
				Debug.DrawRay(transform.position, transform.forward, new Color(0f, 0f, 1f));
			}
		}

		public static PlaneverbListener GetInstance() { return instance; }

		public Vector3 GetPosition()
		{
			return transform.position;
		}

		public Vector3 GetForward()
		{
			return transform.forward;
		}

		public bool HasYChange()
		{
			return Mathf.Abs(oldPosition.y - GetPosition().y) > CHANGE_EPSILON;
		}
	}
} // namespace Planeverb
