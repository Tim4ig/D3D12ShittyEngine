
#pragma once

#ifndef __cplusplus
#error This is C++ engine :)
#endif // __cplusplus

#ifndef _WIN32
#error This engine is for OS Windows only
#endif // !_WIN32

#pragma comment(lib, "SWindowD.lib")
#pragma comment(lib,"OBJLoaderD.lib")

#include "Libs/SWindow.hpp"

#include "MacroDef.hpp"
#include "Renderer.hpp"
#include "RigidBody.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "TimeManager.hpp"
#include "PlayerController.hpp"

namespace engine {
	int UserMain();
}