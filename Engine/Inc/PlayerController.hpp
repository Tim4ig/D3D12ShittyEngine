
#pragma once

#include <Camera.hpp>

namespace engine {
	class PlayerController : public Camera {

	public:
		~PlayerController() { MessageBox(0, L"Destructor call", L"", 0); }
	};
}