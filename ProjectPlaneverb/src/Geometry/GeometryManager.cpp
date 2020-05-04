#include <Geometry\GeometryManager.h>
#include <PvDefinitions.h>
#include <FDTD\Grid.h>
#include <Planeverb.h>
#include <Context\PvContext.h>

namespace Planeverb
{
#pragma region ClientInterface
	PlaneObjectID AddGeometry(const AABB* transform)
	{
		auto* context = GetContext();
		if (context)
		{
			auto* man = context->GetGeometryManager();
			return man->AddObject(transform);
		}
		else
		{
			return PV_INVALID_PLANE_OBJECT_ID;
		}
	}

	void UpdateGeometry(PlaneObjectID id, const AABB* newTransform)
	{
		auto* context = GetContext();
		if (context)
		{
			auto* man = context->GetGeometryManager();
			man->UpdateObject(id, newTransform);
		}
	}

	void RemoveGeometry(PlaneObjectID id)
	{
		auto* context = GetContext();
		if (context)
		{
			auto* man = context->GetGeometryManager();
			man->RemoveObject(id);
		}
	}

#pragma endregion

	GeometryManager::GeometryManager(Grid * grid) :
		m_geometry(), m_openSlots(), m_highestID(0), m_gridPtr(grid), m_geometryChanges()
	{
		// reserve some memory to avoid vector resizing
		m_geometryChanges.reserve(20);
	}

	GeometryManager::~GeometryManager()
	{
		// reset information
		m_geometry.clear();
		m_openSlots.clear();
		m_highestID = 0;
		m_gridPtr = nullptr;
	}

	PlaneObjectID GeometryManager::AddObject(const AABB * box)
	{
		// case no reusable slots left
		if (m_openSlots.empty())
		{
			// add to list of current geometry
			m_geometry.push_back(*box);

			// lock to add to change queue
			GLock lock(m_mutex);
			m_geometryChanges.push_back({ ct_Add, *box });
			return m_highestID++;
		}
		// case reusable slot is available
		else
		{
			// add to list of current geometry
			auto id = m_openSlots.back();
			m_openSlots.pop_back();
			m_geometry[id] = *box;

			// lock to add to change queue
			GLock lock(m_mutex);
			m_geometryChanges.push_back({ ct_Add, *box });
			return id;
		}
	}

	const AABB * GeometryManager::GetPlaneObject(PlaneObjectID id) const
	{
		PV_ASSERT(id != PV_INVALID_PLANE_OBJECT_ID);
		return &(m_geometry[id]);
	}

	void GeometryManager::RemoveObject(PlaneObjectID id)
	{
		PV_ASSERT(id != PV_INVALID_PLANE_OBJECT_ID);

		// lock to add change to change queue
		GLock lock(m_mutex);
		m_geometryChanges.push_back({ ct_Remove, m_geometry[id] });
		std::memset(&(m_geometry[id]), 0, sizeof(AABB));
		m_openSlots.push_back(id);
	}

	void GeometryManager::UpdateObject(PlaneObjectID id, const AABB * transform)
	{
		PV_ASSERT(id != PV_INVALID_PLANE_OBJECT_ID);

		// lock to add a remove and add to change queue
		GLock lock(m_mutex);
		m_geometryChanges.push_back({ ct_Remove, m_geometry[id] });
		m_geometry[id] = *transform;
		m_geometryChanges.push_back({ ct_Add, m_geometry[id] });
	}

	void GeometryManager::PushGeometryChanges()
	{
		// lock to process change queue
		GLock lock(m_mutex);
		size_t size = m_geometryChanges.size();

		// for each change in the queue
		for (size_t i = 0; i < size; ++i)
		{
			// process change in the grid handle
			GeometryChange& next = m_geometryChanges[i];
			switch (next.type)
			{
			case ct_Add:
				m_gridPtr->AddAABB(&next.aabb);
				break;
			case ct_Remove:
				m_gridPtr->RemoveAABB(&next.aabb);
				break;
			}
		}

		// clear change queue
		m_geometryChanges.clear();

		#if PRINT_GRID
			// debug print grid
			m_gridPtr->PrintGrid();
		#endif
	}
} // namespace Planeverb
