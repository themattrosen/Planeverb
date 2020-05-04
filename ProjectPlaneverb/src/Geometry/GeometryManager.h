#pragma once

#include <PvTypes.h>
#include <vector>
#include <mutex>

namespace Planeverb
{
	// Forward declare
	class Grid;

	class GeometryManager
	{
	public:
		GeometryManager(Grid* grid);
		~GeometryManager();
		PlaneObjectID AddObject(const AABB* box);
		const AABB* GetPlaneObject(PlaneObjectID id) const;
		void RemoveObject(PlaneObjectID id);
		void UpdateObject(PlaneObjectID id, const AABB* transform);

		void PushGeometryChanges();

	private:
		// Internal type for geometry changes
		enum ChangeType
		{
			ct_Add,
			ct_Remove,
		};

		// Geometry change internal struct
		struct GeometryChange
		{
			ChangeType type;
			AABB aabb;
		};

		std::vector<AABB> m_geometry;					// keep track of AABBs, object ID is index into vector
		std::vector<PlaneObjectID> m_openSlots;			// list of open AABBs
		PlaneObjectID m_highestID;						// next ID to dispense
		Grid* m_gridPtr;								// handle to the grid

		std::vector<GeometryChange> m_geometryChanges;	// queue of geometry changes to happen at the next sync point
		std::mutex m_mutex;								// sync mutex
		using GLock = std::lock_guard<std::mutex>;		// ease of use typedef for lock_guard
	};
} // namespace Planeverb
