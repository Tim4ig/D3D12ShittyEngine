
#include "Engine.hpp"

int __stdcall wWinMain(HINSTANCE, HINSTANCE, wchar_t*, int) {

	SW::SWindow* pTargetWindow = nullptr;
	engine::Renderer* pRenderer = nullptr;

	pTargetWindow = SW::SWindow::GetSWindowInstance();
	pTargetWindow->OpenAsync(L"Shitty Game Engine!");
	
	pRenderer = engine::Renderer::CreateRendererInstance(pTargetWindow->GetHWND());

	engine::UserMain();

	engine::mem::freeall();

	SW::SWindow::ReleaseSWindowInstance();
	engine::Renderer::DeleteRendererInstance();

	return 0;

}