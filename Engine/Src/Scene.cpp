
#include "Scene.hpp"

namespace engine {

	Scene* g_pTargetScene = nullptr;

	Scene* GetActiveScene()
	{
		return g_pTargetScene;
	}

	void SetNullActiveScene()
	{
		g_pTargetScene = nullptr;
	}

	void Scene::SetAsActive()
	{
		g_pTargetScene = this;
	}

	void Scene::SetClearColor(int color)
	{
		m_ClearColor = color;
	}

	void Scene::SetCamera(Camera& camera)
	{
		m_Camera = camera;
	}

	Camera* Scene::GetCamera()
	{
		return &m_Camera;
	}

	RigidBody* Scene::AddObject(std::string name) {
		if (GetObjectN(name) != &m_MistakeBody) return nullptr;
		auto pObj = new RigidBody;
		m_Obj.push_back(pObj);
		return pObj;
	}

	RigidBody* Scene::CreateObjectFromFile(std::string name, PCWSTR pcwStr)
	{
		auto pObj = this->AddObject(name);
		if (!pObj) return &m_MistakeBody;
		pObj->InitFromFile(pcwStr);
		return pObj;
	}

	RigidBody* Scene::CreateObject(std::string name, engine::Vertex* pRawData, int nVertexCount)
	{
		auto pObj = this->AddObject(name);
		if (!pObj) return &m_MistakeBody;
		pObj->Init(pRawData, nVertexCount);
		return pObj;
	}

	void Scene::RemoveObject(std::string name)
	{
		auto iter = std::find_if(m_Obj.begin(), m_Obj.end(), [&](RigidBody* e) {return e->GetName() == name; });
		m_Obj.erase(iter);
		delete* iter;
	}

	RigidBody* Scene::GetObjectN(std::string name)
	{
		auto it = std::find_if(m_Obj.begin(), m_Obj.end(),
			[&](RigidBody* obj) { return obj->GetName() == name; });

		return ((it != m_Obj.end()) ? *it._Ptr : &m_MistakeBody);
	}

	std::vector<engine::RigidBody*>& Scene::GetObjects()
	{
		return m_Obj;
	}

	Scene::Scene()
	{
		m_MistakeBody.m_bWasInit = 1;
	}

	Scene::~Scene()
	{
		for (auto e : m_Obj) {
			delete e;
		}
		if (GetActiveScene() == this) SetNullActiveScene();
	}

}