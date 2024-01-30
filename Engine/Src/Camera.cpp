
#include "Camera.hpp"

namespace engine {

	void Camera::SetPos(glm::vec3 pos)
	{
		m_bNeedUpdate = true;
		m_Pos = pos;
	}

	void Camera::SetGaze(glm::vec3 target)
	{
		m_bNeedUpdate = true;
		m_Target = target;
	}

}