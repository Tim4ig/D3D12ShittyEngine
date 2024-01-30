
#include "TimeManager.hpp"

namespace engine {

	void TimeManager::Update()
	{
		auto now = std::chrono::high_resolution_clock::now();
		m_DeltaT = (now - m_PreviusT);
		m_PreviusT = now;
	}

	uint32_t TimeManager::GetDTms() const
	{
		return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(m_DeltaT).count());
	}

	float TimeManager::GetFPS() const
	{
		return 1000.f / static_cast<float>(GetDTms());
	}

}