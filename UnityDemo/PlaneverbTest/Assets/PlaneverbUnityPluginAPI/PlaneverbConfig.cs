using UnityEngine;

namespace Planeverb
{
	public enum GridResolution
	{
		pv_LowResolution = 275,
		pv_MidResolution = 375,
		pv_HighResolution = 500,
		pv_ExtremeResolution = 750,

		pv_DefaultResolution = pv_MidResolution
	}

	public enum BoundaryType
	{
		pv_AbsorbingBoundary = 0,
		pv_ReflectingBoundary = 1, // !!! Not supported !!!

		pv_DefaultBoundary = pv_AbsorbingBoundary
	}

	public enum ThreadExecutionType
	{
		pv_CPU = 0,
		pv_GPU = 1, // !!! Not supported !!!

		pv_DefaultThreadExecution = pv_CPU
	}

	[System.Serializable]
	public class PlaneverbConfig
	{
		[Tooltip("Max size for your region of acoustic space.")]
		public Vector2 gridSizeInMeters;

		[Tooltip("Determines how accurate the grid is. Higher resolutions incur higher delays between Planeverb updates.")]
		public GridResolution gridResolution;

		[Tooltip("Determines the type of Absorbing Boundary Condition on the Grid. Currently only Absorbing Boundaries are supported.")]
		public BoundaryType gridBoundaryType;

		[Tooltip("Directory to store cached output files to. Must be a valid directory.")]
		public string tempFileDirectory;

		[Tooltip("Number of threads that Planeverb is allowed to use. Set to 0 to use as many as available.")]
		public int maxThreadUsage;

		[Tooltip("Determines whether Planeverb will execute on the CPU or on the GPU. Currently only the CPU implementation is supported.")]
		public ThreadExecutionType threadExecutionType;
	}

} // namespace Planeverb
