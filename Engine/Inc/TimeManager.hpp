
#pragma once

#include <chrono>

namespace engine {

	class TimeManager {

	public:

		void		Update();
		uint32_t	GetDTms() const;
		float		GetFPS() const;

	private:

		std::chrono::steady_clock::time_point	m_PreviusT;
		std::chrono::steady_clock::duration		m_DeltaT;

	};

}