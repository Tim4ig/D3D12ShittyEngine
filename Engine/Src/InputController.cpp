
#include "InputController.hpp"

#include "Libs/SWindow.hpp"
#include "MacroDef.hpp"

#include <Xinput.h>

#pragma comment(lib, "XInput.lib")

namespace engine::InputController {

	SW::SWindow*	g_Wnd = nullptr;
	glm::vec2		g_MousePos;
	glm::vec2		g_MouseDT;
	glm::vec2		g_MouseCenter;
	bool			g_MouseLock = 0;
	XboxController	g_ControllerState = {};

	glm::vec2 GetWindowCenter();
	void SetMousePos(glm::vec2 pos);
	void ProcssController();

	void ProcessInput()
	{
		if (!g_Wnd) g_Wnd = ::SW::SWindow::GetSWindowInstance();

		POINT pos;
		GetCursorPos(&pos);
		g_MousePos = glm::vec2(pos.x, pos.y);

		if (g_MouseCenter == glm::vec2(0)) g_MouseCenter = GetWindowCenter();

		g_MouseDT = g_MousePos - g_MouseCenter;

		if (g_MouseLock) {
			SetMousePos(g_MouseCenter);
		}

		ProcssController();

	}

	bool* GetKeyArray()
	{
		if(!g_Wnd)
		return nullptr;
		return g_Wnd->GetKeyArray();
	}

	XboxController GetControllerState()
	{
		return g_ControllerState;
	}

	glm::vec2 GetMousePose()
	{
		return g_MousePos;
	}

	glm::vec2 GetMouseDelta()
	{
		return g_MouseDT;
	}

	glm::vec2 GetWindowCenter()
	{
		RECT rect;
		GetWindowRect(g_Wnd->GetHWND(), &rect);

		int centerX = (float)(rect.left + rect.right) / 2.f;
		int centerY = (float)(rect.top + rect.bottom) / 2.f;

		return glm::vec2(centerX, centerY);
	}

	void SetMousePos(glm::vec2 pos) {
		SetCursorPos(pos.x, pos.y);
	}

	void ProcssController()
	{
		XINPUT_STATE state = {};

		for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i) {
			if (XInputGetState(i, &state) == ERROR_SUCCESS) {
				if (state.Gamepad.wButtons & XINPUT_GAMEPAD_A) {
				}
				g_ControllerState.thumbL = glm::vec2((float)state.Gamepad.sThumbLX / 32767.f, (float)state.Gamepad.sThumbLY / 32767.f);
				g_ControllerState.thumbR = glm::vec2((float)state.Gamepad.sThumbRX / 32767.f, (float)state.Gamepad.sThumbRY / 32767.f);
				g_ControllerState.triggers = glm::vec2((float)state.Gamepad.bLeftTrigger/255.f, (float)state.Gamepad.bRightTrigger / 255.f);
			}
			break;
		}

		g_ControllerState.thumbL.x = (fabs(g_ControllerState.thumbL.x) < DEAD_ZONE_THRESHOLD) ? 0 : (g_ControllerState.thumbL.x-DEAD_ZONE_THRESHOLD)/(1- DEAD_ZONE_THRESHOLD);
		g_ControllerState.thumbL.y = (fabs(g_ControllerState.thumbL.y) < DEAD_ZONE_THRESHOLD) ? 0 : (g_ControllerState.thumbL.y-DEAD_ZONE_THRESHOLD)/(1- DEAD_ZONE_THRESHOLD);

		g_ControllerState.thumbR.x = (fabs(g_ControllerState.thumbR.x) < DEAD_ZONE_THRESHOLD) ? 0 : (g_ControllerState.thumbR.x-DEAD_ZONE_THRESHOLD)/(1- DEAD_ZONE_THRESHOLD);
		g_ControllerState.thumbR.y = (fabs(g_ControllerState.thumbR.y) < DEAD_ZONE_THRESHOLD) ? 0 : (g_ControllerState.thumbR.y-DEAD_ZONE_THRESHOLD)/(1- DEAD_ZONE_THRESHOLD);

	}

}
