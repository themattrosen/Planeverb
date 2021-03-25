#pragma once
#include <cmath>
#include <type_traits>

namespace Planeverb
{
	// simple custom array for use by the grid
	template<typename T>
	class Array
	{
	public:
		Array(int count, char* mem = nullptr)
			: m_array(nullptr)
			, m_count(count)
			, m_ownsMemory(mem == nullptr)
			, m_cachedIndex(0)
			, m_cachedValue(nullptr)
		{
			static_assert(std::is_trivially_destructible<T>::value, "Array type is not trivially destructible");

			if (mem)
			{
				m_array = reinterpret_cast<T*>(mem);
			}
			else
			{
				m_array = new T[count];
			}
			m_cachedValue = m_array;
		}

		~Array()
		{
			if (m_ownsMemory)
			{
				delete[] m_array;
				m_array = nullptr;
			}
			m_cachedValue = nullptr;
		}

		inline T& operator[](const int index)
		{
			if (index == m_cachedIndex)
			{
				return *m_cachedValue;
			}
			else if (index == m_cachedIndex + 1)
			{
				++m_cachedIndex;
				return *(++m_cachedValue);
			}
			else
			{
				m_cachedIndex = index;
				m_cachedValue = &m_array[m_cachedIndex];
				return *m_cachedValue;
			}
		}

		inline const T& operator[](const int index) const
		{
			if (index == m_cachedIndex)
			{
				return *m_cachedValue;
			}
			else if (index == m_cachedIndex + 1)
			{
				++m_cachedIndex;
				return *(++m_cachedValue);
			}
			else
			{
				m_cachedIndex = index;
				m_cachedValue = m_array[m_cachedIndex];
				return *m_cachedValue;
			}
		}

		inline T* data()
		{
			return m_array;
		}

	private:
		T* m_array;
		int m_count;
		bool m_ownsMemory;

		mutable int m_cachedIndex;
		mutable T* m_cachedValue;
	};
}
