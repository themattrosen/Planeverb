using System.Runtime.InteropServices;
using UnityEngine;

namespace Planeverb
{
	[StructLayout(LayoutKind.Sequential)]
	public struct PlaneverbOutput
	{
		public float occlusion;
		public float wetGain;
		public float rt60;
		public float lowpass;
		public float directionX;
		public float directionY;
		public float sourceDirectionX;
		public float sourceDirectionY;
	}

	[AddComponentMenu("Planeverb/PlaneverbContext")]
	public class PlaneverbContext : MonoBehaviour
	{
		#region DLL Interface
		private const string DLLNAME = "ProjectPlaneverbUnityPlugin";

		[DllImport(DLLNAME)]
		private static extern void PlaneverbInit(float gridSizeX, float gridSizeY,
		int gridResolution, int gridBoundaryType, string tempFileDir,
		int maxThreadUsage, int threadExecutionType);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbExit();

		[DllImport(DLLNAME)]
		private static extern int PlaneverbEmit(float x, float y, float z);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbUpdateEmission(int id, float x, float y, float z);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbEndEmission(int id);

		[DllImport(DLLNAME)]
		private static extern PlaneverbOutput PlaneverbGetOutput(int emissionID);

		[DllImport(DLLNAME)]
		private static extern int PlaneverbAddGeometry(float posX, float posY,
		float width, float height,
		float absorption);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbUpdateGeometry(int id,
		float posX, float posY,
		float width, float height,
		float absorption);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbRemoveGeometry(int id);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbSetListenerPosition(float x, float y, float z);
		#endregion

		#region MonoBehaviour Overloads
		public PlaneverbConfig config;
		public bool debugDraw = false;
		private static PlaneverbContext contextInstance = null;

		private void Awake()
		{
			PlaneverbInit(config.gridSizeInMeters.x, config.gridSizeInMeters.y,
				(int)config.gridResolution, (int)config.gridBoundaryType,
				config.tempFileDirectory,
				config.maxThreadUsage, (int)config.threadExecutionType);

			Debug.AssertFormat(contextInstance == null, "More than one instance of the PlaneverbContext created! Singleton violated.");
			contextInstance = this;
		}

		void OnDestroy()
		{
			PlaneverbExit();
		}
		#endregion

		#region Static Interface
		public static PlaneverbContext GetInstance()
		{
			return contextInstance;
		}

		public static int Emit(Vector3 pos)
		{
			return PlaneverbEmit(pos.x, pos.y, pos.z);
		}

		public static void UpdateEmission(int id, Vector3 pos)
		{
			PlaneverbUpdateEmission(id, pos.x, pos.y, pos.z);
		}

		public static void EndEmission(int id)
		{
			PlaneverbEndEmission(id);
		}

		public static int AddGeometry(AABB aabb)
		{
			return PlaneverbAddGeometry(aabb.position.x, aabb.position.y,
				aabb.width, aabb.height, aabb.absorption);
		}

		public static void UpdateGeometry(int id, AABB aabb)
		{
			PlaneverbUpdateGeometry(id, aabb.position.x, aabb.position.y,
				aabb.width, aabb.height, aabb.absorption);
		}

		public static void RemoveGeometry(int id)
		{
			PlaneverbRemoveGeometry(id);
		}

		public static void SetListenerPosition(Vector3 position)
		{
			PlaneverbSetListenerPosition(position.x, position.y, position.z);
		}

		public static PlaneverbOutput GetOutput(int emissionID)
		{
			return PlaneverbGetOutput(emissionID);
		}
		#endregion
	}
}