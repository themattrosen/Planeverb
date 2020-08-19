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
		Test,

		Count
	}


	[AddComponentMenu("Planeverb/PlaneverbObject")]
	public class PlaneverbObject : MonoBehaviour
	{
		// user designable parameter: absorption coefficient
		public AbsorptionCoefficient absorption;
		private const float SIZE_EPSILON = 0.01f;
		private const int INVALID_ID = -1;

		// internal geometry id
		private int id = INVALID_ID;
		private bool isInHeadSlice = false;
		private Bounds bounds;

		void Start()
		{
			// calculate AABB and add to context
			bounds = GetMaxBounds();
			isInHeadSlice = IsWithinPlayerHeadSlice(bounds);
			if (isInHeadSlice)
			{
				AABB properties = CalculateAABB(bounds);
				id = PlaneverbContext.AddGeometry(properties);
			}
		}

		void Update()
		{
			// only add if object is at player head slice
			bounds = GetMaxBounds();
			isInHeadSlice = IsWithinPlayerHeadSlice(bounds);

			if (isInHeadSlice)
			{
				// calculate the AABB
				AABB properties = CalculateAABB(bounds);

				// update geometry in the context
				if (id == INVALID_ID)
				{
					id = PlaneverbContext.AddGeometry(properties);
				}
				else
				{
					PlaneverbContext.UpdateGeometry(id, properties);
				}
			}
			else if(id != INVALID_ID)
			{
				PlaneverbContext.RemoveGeometry(id);
				id = INVALID_ID;
			}

			if(PlaneverbContext.GetInstance().debugDraw)
			{
				Color drawColor;
				if (isInHeadSlice)	drawColor = new Color(0f, 1f, 0f);
				else				drawColor = new Color(1f, 0f, 0f);

				// debug draw the bounds
				Vector3 c = bounds.center;
				Vector3 e = bounds.extents; // half extents
				Vector3 bbl = c - e; // bottom back left
				Vector3 bbr = new Vector3(c.x + e.x, c.y - e.y, c.z - e.z); // bottom back right
				Vector3 tbl = new Vector3(c.x - e.x, c.y + e.y, c.z - e.z); // top back left
				Vector3 tbr = new Vector3(c.x + e.x, c.y + e.y, c.z - e.z); // top back right
				Vector3 bfl = new Vector3(c.x - e.x, c.y - e.y, c.z + e.z); // bottom front left
				Vector3 bfr = new Vector3(c.x + e.x, c.y - e.y, c.z + e.z); // bottom front right
				Vector3 tfl = new Vector3(c.x - e.x, c.y + e.y, c.z + e.z); // top front left
				Vector3 tfr = c + e; // top front right

				// connect all the points
				Debug.DrawLine(bbl, bbr, drawColor);
				Debug.DrawLine(bbl, bfl, drawColor);
				Debug.DrawLine(bbl, tbl, drawColor);
				Debug.DrawLine(bbr, tbr, drawColor);
				Debug.DrawLine(bbr, bfr, drawColor);
				Debug.DrawLine(bfl, bfr, drawColor);
				Debug.DrawLine(bfl, tfl, drawColor);
				Debug.DrawLine(bfr, tfr, drawColor);
				Debug.DrawLine(tbl, tbr, drawColor);
				Debug.DrawLine(tbl, tfl, drawColor);
				Debug.DrawLine(tbr, tfr, drawColor);
				Debug.DrawLine(tfl, tfr, drawColor);
			}
		}

		void OnDestroy()
		{
			// remove from context when object is destroyed
			if (id != INVALID_ID)
			{
				PlaneverbContext.RemoveGeometry(id);
			}
		}

		private AABB CalculateAABB(Bounds bounds)
		{
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
		private Bounds GetMaxBounds()
		{
			Bounds b = new Bounds(gameObject.transform.position, Vector3.zero);
			foreach (Collider r in gameObject.GetComponentsInChildren<Collider>())
			{
				b.Encapsulate(r.bounds);
			}
			return b;
		}

		bool IsWithinPlayerHeadSlice(Bounds b)
		{
			float y = PlaneverbListener.GetInstance().GetPosition().y;
			float thisY = b.center.y;
			float halfHeight = b.extents.y; // extents are half sizes
			return (thisY - halfHeight) <= y && (thisY + halfHeight) >= y;
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
			0.994987437F,	// Ice,
			0.97f			// Test
		};	
	}
} // namespace PlaneverCountb
