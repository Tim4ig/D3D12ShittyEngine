
#include "PlayerController.hpp"
#include "Libs/SWindow.hpp"
#include "InputController.hpp"

#define STRAFE_SPEED 0.01

#define CAMERA_CONTROLLER_MULTIPLER 4.25

namespace engine {

	void PlayerController::Tick()
	{
		m_Timemanager.Update();

		bool* pKeys = engine::InputController::GetKeyArray();

		auto posOffset = glm::vec3();
		posOffset.z = (float)(AXIS(KEY_W, KEY_S) * STRAFE_SPEED * m_Timemanager.GetDTms());
		posOffset.x = (float)(AXIS(KEY_D, KEY_A) * STRAFE_SPEED * m_Timemanager.GetDTms());

		posOffset.x = (float)(engine::InputController::GetControllerState().thumbL.x * STRAFE_SPEED * m_Timemanager.GetDTms());
		posOffset.z = (float)(engine::InputController::GetControllerState().thumbL.y * STRAFE_SPEED * m_Timemanager.GetDTms());

		{	
			static float yaw = -90;
			static float pitch = 0;

			float xoffset = engine::InputController::GetMouseDelta().x;
			float yoffset = engine::InputController::GetMouseDelta().y;

			xoffset =  engine::InputController::GetControllerState().thumbR.x*CAMERA_CONTROLLER_MULTIPLER;
			yoffset = -engine::InputController::GetControllerState().thumbR.y*CAMERA_CONTROLLER_MULTIPLER;

			const float sensitivity = 0.05f;
			xoffset *= sensitivity;
			yoffset *= sensitivity;

			yaw -= xoffset;
			pitch += yoffset;

			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;

			glm::vec3 direction;
			direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			direction.y = sin(glm::radians(pitch));
			direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			m_Target = glm::normalize(direction);
		}

		glm::vec3 forward = glm::normalize(glm::vec3(m_Target.x, 0.0f, m_Target.z));
		m_Pos -= forward * posOffset.z;
		m_Pos += glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f))) * posOffset.x;

		m_Pos.y += (float)(AXIS(VK_SPACE, VK_SHIFT) * STRAFE_SPEED * m_Timemanager.GetDTms());

		m_Pos.y -= (engine::InputController::GetControllerState().triggers.x + -engine::InputController::GetControllerState().triggers.y)* STRAFE_SPEED *m_Timemanager.GetDTms();

		m_bNeedUpdate = true;
	}

}