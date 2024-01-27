

#include "Engine.hpp"

#include <dxgidebug.h>
#pragma comment(lib,"dxguid.lib")

int wWinMain(HINSTANCE, HINSTANCE, wchar_t*, int) {

	SW::SWindow* pWnd = SW::SWindow::GetSWindowInstance();
	pWnd->OpenAsync(L"Window");

	engine::Renderer* pRnd = engine::Renderer::CreateRendererInstance(pWnd->GetHWND());

	engine::Scene* pSc = new engine::Scene;

	pSc->CreateObjectFromFile("name", L"d/8.obj");
	pSc->GetObjectN("name")->SetPos(glm::vec3(0, 0, 3));
	pSc->SetAsActive();

	int anim = 0;

	while (pWnd->IsOpen()) {
		pSc->GetObjectN("name")->SetAngle(glm::vec3(0, (float)++anim/1000.f, 0));

		if (pWnd->IsResize()) pRnd->Resize();

		pRnd->Begin();
		pRnd->Clear();
		pRnd->DrawScene();
		pRnd->End();
	}

	delete pSc;

	engine::Renderer::DeleteRendererInstance();
	SW::SWindow::ReleaseSWindowInstance();

	return 0;
}