using System.Runtime.InteropServices;
using System;
using UnityEngine;

namespace Planeverb
{
	[StructLayout(LayoutKind.Sequential)]
	public struct PlaneverbDSPInput
	{
		public float obstructionGain;
		public float wetGain;
		public float rt60;
		public float lowpass;
		public float directionX;
		public float directionY;
		public float sourceDirectionX;
		public float sourceDirectionY;
	}

	[AddComponentMenu("Planeverb/DSP/PlaneverbDSPContext")]
	public class PlaneverbDSPContext : MonoBehaviour
	{
		#region DLL Interface
		private const string DLLNAME = "PlaneverbDSPUnityPlugin";

		[DllImport(DLLNAME)]
		private static extern void PlaneverbDSPInit(int maxCallbackLength, int samplingRate,
		int dspSmoothingFactor, bool useSpatialization,
		float wetGainRatio, int maxEmitters);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbDSPExit();

		[DllImport(DLLNAME)]
		private static extern void PlaneverbDSPSetListenerTransform(
			float posX, float posY, float posZ,
			float forwardX, float forwardY, float forwardZ,
			float upX, float upY, float upZ
		);

		[DllImport(DLLNAME)]
		private static extern int PlaneverbDSPAddEmitter(
			float posX, float posY, float posZ,
			float forwardX, float forwardY, float forwardZ,
			float upX, float upY, float upZ
		);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbDSPUpdateEmitter(int emissionID,
			float posX, float posY, float posZ,
			float forwardX, float forwardY, float forwardZ,
			float upX, float upY, float upZ
		);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbDSPRemoveEmitter(int emissionID);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbDSPSetEmitterDirectivityPattern(int emissionId, int pattern);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbDSPSendSource(int emissionID, in PlaneverbDSPInput dspParams,
		float[] input, int numFrames);

		[DllImport(DLLNAME)]
		private static extern bool PlaneverbDSPProcessOutput();

		[DllImport(DLLNAME)]
		private static extern void PlaneverbDSPGetDryBuffer(ref IntPtr buf);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbDSPGetBufferA(ref IntPtr ptrArray);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbDSPGetBufferB(ref IntPtr ptrArray);

		[DllImport(DLLNAME)]
		private static extern void PlaneverbDSPGetBufferC(ref IntPtr ptrArray);

		private delegate void FetchOutputBuffer(ref IntPtr ptrArray);
		private static FetchOutputBuffer[] outputFetchers =
		{
			PlaneverbDSPGetDryBuffer,
			PlaneverbDSPGetBufferA,
			PlaneverbDSPGetBufferB,
			PlaneverbDSPGetBufferC
		};

		#endregion

		#region MonoBehaviour Overloads
		public PlaneverbDSPConfig config;

		public static int MAX_FRAME_LENGTH = 4096;
		public static PlaneverbDSPContext globalContext = null;

		private void Awake()
		{
			globalContext = this;

			PlaneverbDSPInit(config.maxCallbackLength, config.samplingRate,
				config.dspSmoothingFactor, config.useSpatialization,
				config.wetGainRatio, config.maxEmitters
			);
		}

		void OnDestroy()
		{
			PlaneverbDSPExit();
		}

		#endregion

		#region Static Interface
		public static void SetListenerTransform(Vector3 position, Vector3 forward, Vector3 up)
		{
			PlaneverbDSPSetListenerTransform(position.x, position.y, position.z,
				forward.x, forward.y, forward.z,
				up.x, up.y, up.z
			);
		}

		public static int AddEmitter(Vector3 pos, Vector3 forward, Vector3 up)
        {
			return PlaneverbDSPAddEmitter(pos.x, pos.y, pos.z,
				forward.x, forward.y, forward.z,
				up.x, up.y, up.z
			);
        }

		public static void UpdateEmitter(int id, Vector3 pos, Vector3 forward, Vector3 up)
		{
			PlaneverbDSPUpdateEmitter(id, 
				pos.x, pos.y, pos.z, 
				forward.x, forward.y, forward.z,
				up.x, up.y, up.z
			);
		}

		public static void RemoveEmitter(int id)
        {
			PlaneverbDSPRemoveEmitter(id);
        }

		public static void SetEmitterDirectivityPattern(int id, PlaneverbSourceDirectivityPattern pattern)
		{
			PlaneverbDSPSetEmitterDirectivityPattern(id, (int)pattern);
		}

		public static void SendSource(int id, in PlaneverbDSPInput param, float[] data, int numSamples, int channels)
		{
			int frames = numSamples / channels;
			PlaneverbDSPSendSource(id, param, data, frames);
		}

		public static bool ProcessOutput()
		{
			return PlaneverbDSPProcessOutput();
		}

		public static void GetOutputBuffer(int reverb, ref float[] buff)
		{
			// ensure that the index is valid
			if (reverb < 0 || reverb >= 4) return;
			
			// fetch the buffer
			IntPtr result = IntPtr.Zero;
			outputFetchers[reverb](ref result);
			
			// copy the buffer as a float array
			Marshal.Copy(result, buff, 0, MAX_FRAME_LENGTH);
		}

		#endregion

	}
}
