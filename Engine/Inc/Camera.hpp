
#pragma once

#include "MacroDef.hpp"

namespace engine {
	class Camera {
	public:
		void SetPos(glm::vec3 pos);
		void SetGaze(glm::vec3 target);
	protected:
		bool m_bNeedUpdate = true;
		glm::vec3 m_Pos = glm::vec3(0);
		glm::vec3 m_Target = glm::vec3(0, 0, -1);
		glm::mat4 m_View;
		friend class Renderer;
	};
}