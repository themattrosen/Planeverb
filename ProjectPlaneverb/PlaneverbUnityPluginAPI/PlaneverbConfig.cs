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

	public enum GridCenteringType
	{
		pv_StaticCentering,
		pv_DynamicCentering
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

		[Tooltip("The acoustic grid can either center itself statically at a point, or follow the listener dynamically.")]
		public GridCenteringType gridCenteringType;

		[Tooltip("Offset from the 'center' in meters. For static centering, offset is from the origin, for dynamic centering, offset is off of the listener position.")]
		public Vector2 gridWorldOffset;
	}

} // namespace Planeverb
