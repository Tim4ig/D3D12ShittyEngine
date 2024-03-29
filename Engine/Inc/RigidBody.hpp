
#pragma once

#include "MacroDef.hpp"
#include <D3DX12/d3dx12.h>

namespace engine {

	struct Vertex {
		float x, y, z;
		float nx, ny, nz;
	};

	struct BodyShaderData {
		glm::mat4x4 model;
	};

	class RigidBody {

	public:

		void InitFromFile(PCWSTR pcwFileName);
		void Init(Vertex* pData, UINT countOfVertex);
		void SetEffect(int nEffect);

		void SetRotationRad(glm::vec3 angle);
		void SetRotationDeg(glm::vec3 angle);
		void SetPos(glm::vec3 pos);

		std::string GetName();

		~RigidBody();

	private:

		void                        m_Update();

		std::string                    m_Name;

		bool                        m_bWasInit = 0;
		Vertex* m_pRawData = nullptr;

		UINT                        m_nEffect = 0;

		ComPtr<ID3D12Resource>        m_CBOBuffer;
		D3D12_VERTEX_BUFFER_VIEW    m_View = {};
		D3D12_GPU_DESCRIPTOR_HANDLE    m_Handle = {};

		std::vector<std::pair<ComPtr<ID3D12Resource>, D3D12_VERTEX_BUFFER_VIEW>> m_VertexBuffers;

		glm::vec3                    m_ModelPos;
		glm::vec3                    m_ModelAngles;
		bool                        m_NeedUpdate = false;

		BodyShaderData                m_LocalShaderData;

		friend class Renderer;
		friend class Scene;

	};

}