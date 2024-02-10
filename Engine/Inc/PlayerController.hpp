
#pragma once

#include "Camera.hpp"
#include "TimeManager.hpp"

namespace engine {
	class PlayerController : public Camera {
	public:

		void Tick();

	private:
		TimeManager m_Timemanager;
	};
}