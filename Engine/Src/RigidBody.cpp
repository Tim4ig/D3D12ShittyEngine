 
#include "RigidBody.hpp"

#include "libs/OBJFileLoader2.hpp"
#include "MacroDef.hpp"
#include "Renderer.hpp"

namespace engine {

	void CreateVertexData(loader::Mesh& m, engine::Vertex** pData, UINT* nVertexCount);

	void RigidBody::InitFromFile(PCWSTR pcwFileName) {

		if (m_bWasInit) return;

		loader::File data;
		bool res = loader::LoadOBJ(pcwFileName, &data);
		if (!res) return;

		UINT count = static_cast<UINT>(data.meshes.size());

		for (UINT i = 0; i < count; ++i) {
			engine::Vertex* pVertex = nullptr;
			UINT			nCount = 0;
			CreateVertexData(data.meshes[i], &pVertex, &nCount);
			Init(pVertex, nCount);
			delete[] pVertex;
		}

		data.Release();

	}

	void RigidBody::Init(Vertex* pData, UINT countOfVertex)
	{

		if (m_bWasInit) return;
		

		HRESULT hr = S_OK;

		Renderer* pRnd = Renderer::GetRendererInstance();
		if (!pRnd) throw_err("Renderer was nullptr");
		ID3D12Device5* pDev = pRnd->GetDevice();
		if (!pDev) throw_err("Device was nullptr");

		m_pRawData = new Vertex[countOfVertex];
		memcpy(m_pRawData, pData, countOfVertex * sizeof(Vertex));

		ID3D12Resource* pVertexBuffer = nullptr;
		D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {};

		//Create vertex buffer
		{
			CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
			CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(countOfVertex * sizeof(Vertex));
			hr = pDev->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pVertexBuffer)) throw_if_err;

			BYTE* pVertexData = nullptr;
			CD3DX12_RANGE readRange(0, 0);

			pVertexBuffer->Map(0, &readRange, (void**)&pVertexData);
			if (pVertexData) memcpy(pVertexData, pData, countOfVertex * sizeof(Vertex));
			pVertexBuffer->Unmap(0, nullptr);
		}

		VertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
		VertexBufferView.SizeInBytes = countOfVertex * sizeof(Vertex);
		VertexBufferView.StrideInBytes = sizeof(Vertex);

		m_VertexBuffers.push_back(std::make_pair(pVertexBuffer, VertexBufferView));
		int a = pVertexBuffer->Release();

		//Create cbo buffer
		{
			D3D12_HEAP_PROPERTIES heapProps = {};
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProps.CreationNodeMask = 1;
			heapProps.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC cbResourceDesc = {};
			cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			cbResourceDesc.Alignment = 0;
			cbResourceDesc.Width = (sizeof(engine::BodyShaderData) + 255) & ~255;
			cbResourceDesc.Height = 1;
			cbResourceDesc.DepthOrArraySize = 1;
			cbResourceDesc.MipLevels = 1;
			cbResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			cbResourceDesc.SampleDesc.Count = 1;
			cbResourceDesc.SampleDesc.Quality = 0;
			cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			cbResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			hr = (engine::Renderer::GetRendererInstance()->GetDevice()->CreateCommittedResource(
				&heapProps, D3D12_HEAP_FLAG_NONE, &cbResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_CBOBuffer)));

			m_CBOBuffer->SetName(L"BABA");

			m_NeedUpdate = 1;
		}

		delete[] m_pRawData;
		m_pRawData = nullptr;

	}

	void RigidBody::SetRotationRad(glm::vec3 angle) {
		m_ModelAngles = angle;
		m_NeedUpdate = 1;
	}

	void RigidBody::SetRotationDeg(glm::vec3 angle) {
		this->SetRotationRad(glm::radians(angle));
	}

	void RigidBody::SetPos(glm::vec3 pos) {
		m_ModelPos = pos;
		m_NeedUpdate = 1;
	}

	void RigidBody::m_Update() {
		glm::quat rotationQuaternion = glm::quat(m_ModelAngles);
		m_LocalShaderData.model = glm::translate(m_ModelPos) * glm::mat4_cast(rotationQuaternion);
	}

	void RigidBody::SetEffect(int nEffect)
	{
		m_nEffect = nEffect;
	}

	std::string RigidBody::GetName()
	{
		return m_Name;
	}

	RigidBody::~RigidBody()
	{
		delete m_pRawData;
		m_CBOBuffer.Reset();
		for (auto& e : m_VertexBuffers) {
			int a = e.first.Reset();
			a = 0;
		}
		
	}

	void CreateVertexData(loader::Mesh& m, engine::Vertex** pData, UINT* nVertexCount) {

		*nVertexCount = (int)m.c_f;

		*pData = new Vertex[(*nVertexCount) * 3];

		int pos = 0;

		for (UINT i = 0; i < (*nVertexCount); ++i) {

			int ax[] = {m.pIndex[i].points[0].x - 1,m.pIndex[i].points[0].z - 1};
			int ay[] = {m.pIndex[i].points[1].x - 1,m.pIndex[i].points[1].z - 1};
			int az[] = {m.pIndex[i].points[2].x - 1,m.pIndex[i].points[2].z - 1};

#define rbVLoaderHelper(ss) { \
			(*pData)[pos].x = m.pPoints[ss[0]].x;\
			(*pData)[pos].y = m.pPoints[ss[0]].y;\
			(*pData)[pos].z = m.pPoints[ss[0]].z;\
			(*pData)[pos].nx = m.pNormals[ss[1]].x;\
			(*pData)[pos].ny = m.pNormals[ss[1]].y;\
			(*pData)[pos].nz = m.pNormals[ss[1]].z;\
			pos++; }

			rbVLoaderHelper(ax);
			rbVLoaderHelper(ay);
			rbVLoaderHelper(az);

		}
			(*nVertexCount) *= 3;
			return;
	}
}