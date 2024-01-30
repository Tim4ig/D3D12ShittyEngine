
#include "Engine.hpp"

int engine::UserMain() {

	SW::SWindow* pWnd = SW::SWindow::GetSWindowInstance();
	engine::Renderer* pRnd = engine::Renderer::GetRendererInstance();
	engine::TimeManager CTimeManager;

	auto pScene = mem::malloc<engine::Scene>();
	pScene->SetAsActive();
	pScene->SetClearColor(0x00ff00ff);

	auto pTest = pScene->CreateObjectFromFile("test", L"d/5.obj");
	pTest->SetPos({ 0,0,5 });
	pTest->SetRotationDeg({ 0,45,0 });

	while (pWnd->IsOpen()) {
		CTimeManager.Update();
		pRnd->Begin();
		pRnd->Clear();
		pRnd->DrawScene();
		pRnd->End();
	}

	return 0;
}