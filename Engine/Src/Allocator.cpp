
#include "Allocator.hpp"

#include "Scene.hpp"
#include "Camera.hpp"
#include "RigidBody.hpp"
#include "PlayerController.hpp"

namespace engine {

	std::vector<mem::AllocatedResource*> g_Res;
	
	void mem::free(AllocatedResource* ptr)
	{
		auto iter = std::find_if(g_Res.begin(), g_Res.end(), [&](engine::mem::AllocatedResource* e) {return e == ptr; });
		if (iter == g_Res.end()) return;
		((*iter)->nSize == 1) ? delete (*iter) : delete[](*iter);
		g_Res.erase(iter);
	}

	void mem::freeall()
	{
		for (auto& e : g_Res) (e->nSize == 1) ? delete e : delete[] e;
	}

	template<class T>
	T* mem::malloc(size_t count)
	{
		T* ptr = (count == 1) ? new T : new T[count];
		reinterpret_cast<AllocatedResource*>(ptr)->nSize = count;
		g_Res.push_back(ptr);
		return ptr;
	}

	template engine::Scene* mem::malloc<engine::Scene>(size_t count);
	template engine::Camera* mem::malloc<engine::Camera>(size_t count);
	template engine::RigidBody* mem::malloc<engine::RigidBody>(size_t count);
	template engine::PlayerController* mem::malloc<engine::PlayerController>(size_t count);


}