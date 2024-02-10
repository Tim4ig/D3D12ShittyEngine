
#pragma once

#ifndef __cplusplus
#error This library is for C++ only
#endif // __cplusplus

#ifndef _WIN32
#error This library is for OS Windows only
#endif // !_WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <thread>

namespace SW {

	namespace Modes {
		enum Mode {
			Windowed,
			Borderless,
		};
	}

	class SWindow {
	public:
		//---------------------------------------------------------------------------------------
		static SWindow* GetSWindowInstance();
		static void		ReleaseSWindowInstance();
		//---------------------------------------------------------------------------------------
		void		OpenAsync(PCWSTR cwszWindowTitle, int nCmdShow = SW_SHOWDEFAULT);
		void		Show(int nCmdShow = SW_SHOWDEFAULT);
		//---------------------------------------------------------------------------------------	
		void		SetTitle(PCWSTR cwszWindowTitle);
		void		SetMode(Modes::Mode eMode);
		//---------------------------------------------------------------------------------------
		HWND		GetHWND() const;
		POINT		GetClientSize();
		PCWSTR		GetTitle();
		bool		IsActive() const;
		bool		IsOpen() const;
		bool [[nodiscard]] IsResize();
		bool* GetKeyArray();
		Modes::Mode GetMode() const;
		//---------------------------------------------------------------------------------------
		~SWindow();
	private:
		SWindow() = default;
		static LRESULT __stdcall m_WndProc(HWND hWnd, UINT uMsg, WPARAM wP, LPARAM lP);
		//---------------------------------------------------------------------------------------
		std::thread	m_WindowThread;
		bool		m_bIsOpen = false;
		HWND		m_hWnd = NULL;
		RECT		m_rWndPos = { 0,0,0,0 };
		LONG_PTR	m_unWndStyle = NULL;
		Modes::Mode m_eMode = Modes::Windowed;
		bool		m_bIsResize = false;
		bool		m_pKeys[256] = {};
		//---------------------------------------------------------------------------------------
	};

}