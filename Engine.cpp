

#include "Engine.hpp"

int main() {

	SW::SWindow* pWnd = SW::SWindow::GetSWindowInstance();
	pWnd->OpenAsync(L"Window");

	engine::Renderer* pRnd = engine::Renderer::CreateRendererInstance(pWnd->GetHWND());

	engine::Scene* pSc1 = new engine::Scene;
	pSc1->SetAsActive();

	pSc1->CreateObjectFromFile("test",L"d/6.obj");

	pSc1->GetObjectN("test")->SetPos(glm::vec3(0, -0.2, 5));

	pSc1->SetClearColor(0xff0000ff);

	int anim = 0;

	while (pWnd->IsOpen()) {
		pSc1->GetObjectN("test")->SetAngle(glm::vec3(0, (float)++anim/1000.f, 0));

		pRnd->Begin();
		pRnd->Clear();
		pRnd->DrawScene();
		pRnd->End();
	}

	delete pSc1;

	engine::Renderer::DeleteRendererInstance();
	SW::SWindow::ReleaseSWindowInstance();

	return 0;
}