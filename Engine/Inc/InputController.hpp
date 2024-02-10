
#pragma once

#include "MacroDef.hpp"

#define KEY_W 'W'
#define KEY_A 'A'
#define KEY_S 'S'
#define KEY_D 'D'

#define GET_KEY_STATE(a) (engine::InputController::GetKeyArray()[a])
#define AXIS(a,b) (float)((GET_KEY_STATE(a)) ? 1 : (GET_KEY_STATE(b)) ? -1 : 0)

#define DEAD_ZONE_THRESHOLD 0.12f

namespace engine {

	namespace InputController {

		struct XboxController {
			glm::vec2 thumbL;
			glm::vec2 thumbR;
			glm::vec2 triggers;
		};

		void ProcessInput();

		bool* GetKeyArray();

		XboxController GetControllerState();

		glm::vec2 GetMousePose();
		glm::vec2 GetMouseDelta();
	}

}