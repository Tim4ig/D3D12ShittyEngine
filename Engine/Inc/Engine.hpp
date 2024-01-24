
#pragma once

#ifndef __cplusplus
#error This is C++ engine :)
#endif // __cplusplus

#ifndef _WIN32
#error This engine is for OS Windows only
#endif // !_WIN32

#pragma comment(lib, "SWindow.lib")

#include "Libs/SWindow.hpp"

#include "MacroDef.hpp"
#include "Renderer.hpp"
#include "RigidBody.hpp"
#include "Scene.hpp"