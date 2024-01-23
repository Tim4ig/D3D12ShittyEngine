
#include "Scene.hpp"

namespace engine {

	Scene* g_pTargetScene = nullptr;

	Scene* GetActiveScene()
	{
		return g_pTargetScene;
	}

	void Scene::SetAsActive()
	{
		g_pTargetScene = this;
	}

	void Scene::SetClearColor(int color)
	{
		m_ClearColor = color;
	}

	void Scene::CreateObjectFromFile(std::string name, PCWSTR pcwStr)
	{
		if (GetObjectN(name)) return;
		m_Obj.push_back(new RigidBody);
		int index = m_Obj.size() - 1;
		m_Obj[index]->m_Name = name;
		m_Obj[index]->InitFromFile(pcwStr);
	}

	void Scene::CreateObject(std::string name, engine::Vertex* pRawData, int nVertexCount)
	{
		if (GetObjectN(name)) return;
		m_Obj.push_back(new RigidBody);
		int index = m_Obj.size() - 1;
		m_Obj[index]->m_Name = name;
		m_Obj[index]->Init(pRawData, nVertexCount);
	}

	void Scene::RemoveObject(std::string name)
	{
		m_Obj.erase(std::remove_if(m_Obj.begin(), m_Obj.end(), [&](RigidBody* e) {return e->GetName() == name; }));
	}



	RigidBody* Scene::GetObjectN(std::string name)
	{
		auto it = std::find_if(m_Obj.begin(), m_Obj.end(),
			[&](RigidBody* obj) { return obj->GetName() == name; });

		return ((it != m_Obj.end()) ? *it._Ptr : nullptr);
	}

	std::vector<engine::RigidBody*>& Scene::GetObjects()
	{
		return m_Obj;
	}

	

	Scene::~Scene()
	{
		for (auto e : m_Obj) delete e;
	}

}