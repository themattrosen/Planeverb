using UnityEngine;

namespace Planeverb
{
	[System.Serializable]
	public struct AABB
	{
		// grid position, not world position
		public Vector2 position;

		// full width
		public float width;

		// full height
		public float height;

		// absorption coefficient
		public float absorption;
	}

	// enum for readability in editor, used as an index into a parallel array
	public enum AbsorptionCoefficient
	{
		FreeSpace,
		Default,
		BrickUnglazed,
		BrickPainted,
		ConcreteRough,
		ConcreteBlockPainted,
		GlassHeavy,
		GlassWindow,
		TileGlazed,
		PlasterBrick,
		PlasterConcreteBlock,
		WoodPlywoodPanel,
		Steel,
		WoodPanel,
		ConcreteBlockCoarse,
		DraperyLight,
		DraperyMedium,
		DraperyHeavy,
		FiberboardShreddedWood,
		ConcretePainted,
		Wood,
		WoodVarnished,
		CarpetHeavy,
		Gravel,
		Grass,
		SnowFresh,
		SoilRough,
		WoodTree,
		WaterSurface,
		Concrete,
		Glass,
		Marble,
		Drapery,
		Cloth,
		Awning,
		Foliage,
		Metal,
		Ice,

		Count
	}


	[AddComponentMenu("Planeverb/PlaneverbObject")]
	public class PlaneverbObject : MonoBehaviour
	{
		// user designable parameter: absorption coefficient
		public AbsorptionCoefficient absorption;
		private float SIZE_EPSILON = 0.01f;
		// internal AABB from previous frame stored to reduce calls to update geometry
		private AABB oldAABB;

		// internal geometry id
		private int id;

		void Start()
		{
			// calculate AABB and add to context
			AABB properties = CalculateAABB();
			oldAABB = properties;
			id = PlaneverbContext.AddGeometry(properties);
		}

		void Update()
		{
			// calculate the AABB
			AABB properties = CalculateAABB();

			// case the AABB has changed from the previous frame
			if (properties.position != oldAABB.position || 
				properties.width != oldAABB.width || 
				properties.height != oldAABB.height || 
				properties.absorption != oldAABB.absorption)
			{
				// update geometry in the context
				PlaneverbContext.UpdateGeometry(id, properties);
				oldAABB = properties;
			}
		}

		void OnDestroy()
		{
			// remove from context when object is destroyed
			PlaneverbContext.RemoveGeometry(id);
		}

		private AABB CalculateAABB()
		{
			// calculate its bounds in world space
			Bounds bounds = GetMaxBounds(gameObject);

			// calculate full width and height from half width and half height
			float width = bounds.extents.x * 2f - SIZE_EPSILON;
			float height = bounds.extents.z * 2f - SIZE_EPSILON;

			// create a new AABB and set values
			AABB properties = new AABB();

			// use absorption coefficient enum as index into static absorption array
			properties.absorption = s_absorptionCoefficients[(int)absorption];
			properties.position.x = bounds.center.x;
			properties.position.y = bounds.center.z;
			properties.width = width;
			properties.height = height;
			return properties;
		}

		// helper function taken from gamedev stackexchange:
		// https://gamedev.stackexchange.com/questions/86863/calculating-the-bounding-box-of-a-game-object-based-on-its-children
		// user Alex Merlin
		private Bounds GetMaxBounds(GameObject g)
		{
			Bounds b = new Bounds(g.transform.position, Vector3.zero);
			foreach (Collider r in g.GetComponentsInChildren<Collider>())
			{
				b.Encapsulate(r.bounds);
			}
			return b;
		}

		// parallel array for absorption enum
		private static float[] s_absorptionCoefficients = 
		{
			0.000000000F,	// FreeSpace,
			0.989949494F,	// Default,
			0.979795897F,	// BrickUnglazed,
			0.989949494F,	// BrickPainted,
			0.969535971F,	// ConcreteRough,
			0.964365076F,	// ConcreteBlockPainted,
			0.984885780F,	// GlassHeavy,
			0.938083152F,	// GlassWindow,
			0.994987437F,	// TileGlazed,
			0.984885780F,	// PlasterBrick,
			0.974679434F,	// PlasterConcreteBlock,
			0.948683298F,	// WoodPlywoodPanel,
			0.948683298F,	// Steel,
			0.953939201F,	// WoodPanel,
			0.806225775F,	// ConcreteBlockCoarse,
			0.921954446F,	// DraperyLight,
			0.670820393F,	// DraperyMedium,
			0.632455532F,	// DraperyHeavy,
			0.632455532F,	// FiberboardShreddedWood,
			0.989949494F,	// ConcretePainted,
			0.964365076F,	// Wood,
			0.984885780F,	// WoodVarnished,
			0.806225775F,	// CarpetHeavy,
			0.547722558F,	// Gravel,
			0.547722558F,	// Grass,
			0.316227766F,	// SnowFresh,
			0.741619849F,	// SoilRough,
			0.911043358F,	// WoodTree,
			0.994987437F,	// WaterSurface,
			0.979795897F,	// Concrete,
			0.969535971F,	// Glass,
			0.994987437F,	// Marble,
			0.921954446F,	// Drapery,
			0.921954446F,	// Cloth,
			0.921954446F,	// Awning,
			0.911043358F,	// Foliage,
			0.948683298F,	// Metal,
			0.994987437F	// Ice,
		};	
	}
} // namespace PlaneverCountb
