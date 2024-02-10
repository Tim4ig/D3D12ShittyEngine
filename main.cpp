
#include "Engine.hpp"

int engine::UserMain() {

	SW::SWindow* pWnd = SW::SWindow::GetSWindowInstance();
	engine::Renderer* pRnd = engine::Renderer::GetRendererInstance();
	engine::TimeManager CTimeManager;
	engine::PlayerController pC;

	auto pScene = engine::mem::malloc<engine::Scene>();
	pScene->SetAsActive();

	auto pTest1 = pScene->CreateObjectFromFile("test1", L"d/7.obj");
	pTest1->SetPos({ 0,0,14 });
	pTest1->SetRotationDeg({ 0,0,0 });

	while (pWnd->IsOpen()) {

		engine::InputController::ProcessInput();

		static float anim = 0;
		anim++;

		CTimeManager.Update();
		pC.Tick();

		pScene->SetCamera(pC);
		pRnd->Begin();
		pRnd->Clear();
		pRnd->DrawScene();
		pRnd->End();
		
		if (pWnd->IsResize()) pRnd->Resize();

	}

	return 0;
}