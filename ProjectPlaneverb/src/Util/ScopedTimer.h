#pragma once

#include <chrono>
#include <iostream>
namespace Planeverb
{
	class ScopedTimer
	{
	public:
		ScopedTimer(std::ostream& outStream = std::cout)
			: m_outputStream(outStream)
		{
			Start();
		}

		~ScopedTimer()
		{
			Stop();
		}

		void Start()
		{
			m_startTimePoint = std::chrono::high_resolution_clock::now();
		}

		void Stop()
		{
			auto endTimePoint = std::chrono::high_resolution_clock::now();
			auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_startTimePoint).time_since_epoch().count();
			auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimePoint).time_since_epoch().count();

			auto duration = end - start;
			m_outputStream << duration << "us";
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> m_startTimePoint;
		std::ostream& m_outputStream;
	};
}