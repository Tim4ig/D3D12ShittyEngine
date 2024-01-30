
#pragma once

#include <vector>

#include "MacroDef.hpp"
#include "RigidBody.hpp"
#include "Camera.hpp"

namespace engine {
	
	struct SceneShaderData {
		glm::mat4x4 view;
		glm::mat4x4 projection;
		glm::vec4	pos;
	};

	class Scene : public mem::AllocatedResource {

	public:

		Scene();

		void SetAsActive();

		RigidBody* CreateObjectFromFile(std::string name, PCWSTR pcwStr);
		RigidBody* CreateObject(std::string name, engine::Vertex* pRawData, int nVertexCount);
		void RemoveObject(std::string name);
		RigidBody* GetObjectN(std::string name);

		void SetCamera(Camera& camera);
		Camera* GetCamera();

		void SetClearColor(int color);

		std::vector<engine::RigidBody*>& GetObjects();

		~Scene();

	private:

		RigidBody*						AddObject(std::string name);

		Camera							m_Camera;

		SceneShaderData					m_LocalShaderData = {};

		D3D12_GPU_DESCRIPTOR_HANDLE		m_RCBOHandle = {};
		ComPtr<ID3D12DescriptorHeap>	m_ConstantHeap			= nullptr;
		bool							m_NeedHeapUpdate		= true;
		bool							m_NeedConstantUpdate	= true;
		ComPtr<ID3D12Resource>			m_RCBOBuffer			= nullptr;

		RigidBody						m_MistakeBody;

		int m_ClearColor = 0x000000ff;

	private:

		std::vector<engine::RigidBody*> m_Obj;
		friend class Renderer;

	};

	Scene*	GetActiveScene();
	void	SetNullActiveScene();

}