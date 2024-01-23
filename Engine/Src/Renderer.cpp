
#include "Renderer.hpp"

#include <vector>
#include <functional>

#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#include "glm/gtc/matrix_transform.hpp"

namespace engine {

	Renderer* g_pRendererInstance = nullptr;

	std::pair<ID3DBlob*, ID3DBlob*> g_LoadShaders(LPCWSTR pcwFileName);

	Renderer* Renderer::CreateRendererInstance(HWND hWnd)
	{
		if (g_pRendererInstance) return g_pRendererInstance;
		g_pRendererInstance = new Renderer;
		g_pRendererInstance->m_InitD3D12(hWnd);
		return g_pRendererInstance;
	}

	Renderer* Renderer::GetRendererInstance()
	{
		return g_pRendererInstance;
	}

	void Renderer::DeleteRendererInstance()
	{
		delete g_pRendererInstance;
	}

	void Renderer::Begin()
	{
		for(auto allocator : m_Allocators) allocator->Reset();
		
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_RTVs[m_nRTVIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDesc(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), m_nRTVIndex, m_nRTVDescSize);

		for (int i = 0; i < m_nEffectCount; ++i)
		{
			m_Lists[i]->Reset(m_Allocators[i].Get(), m_Effects[i].m_PipelineState.Get());
			m_Lists[i]->SetGraphicsRootSignature(m_Effects[i].m_Signature.Get());
			m_Lists[i]->RSSetViewports(1, &m_ViewPort);
			m_Lists[i]->RSSetScissorRects(1, &m_ScissorRect);
			m_Lists[i]->ResourceBarrier(1, &barrier);
			m_Lists[i]->OMSetRenderTargets(1, &rtvDesc, FALSE, nullptr);
			m_Lists[i]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}

	}

	void Renderer::Clear()
	{

			CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDesc(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), m_nRTVIndex, m_nRTVDescSize);

			int ClearColor = 0;
			
			Scene* pScene = GetActiveScene();

			if (pScene) ClearColor = pScene->m_ClearColor;

			float r = ((ClearColor >> 24) & 255) / 255.f;
			float g = ((ClearColor >> 16) & 255) / 255.f;
			float b = ((ClearColor >> 8) & 255) / 255.f;
			float a = ((ClearColor >> 0) & 255) / 255.f;
			float clr[] = { r,g,b,a };
			m_Lists.at(0)->ClearRenderTargetView(rtvDesc, clr, 0, nullptr);
		
	}

	void Renderer::End()
	{

		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_RTVs[m_nRTVIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		for (auto eff : m_Lists)
		{
			eff->ResourceBarrier(1, &barrier);
			eff->Close();
		}
	
		ID3D12CommandList** ppLists = (ID3D12CommandList**)(m_Lists[0].GetAddressOf());

		m_Queue->ExecuteCommandLists(m_Effects.size(), ppLists);
		m_SwapChain->Present(1, 0);

		m_Sync();
	}

	ID3D12Device5* Renderer::GetDevice()
	{
		m_Device->AddRef();
		return m_Device.Get();
	}

	void Renderer::DrawScene()
	{
		engine::Scene * pScene =  engine::GetActiveScene();
		if (!pScene) return;

		m_UpdateCbuffer(pScene);

		
		for (int i = 0; i < m_Effects.size(); ++i) {
			ID3D12DescriptorHeap* pDescriptorHeaps[] = { pScene->m_ConstantHeap.Get() };
			m_Lists[i]->SetDescriptorHeaps(_countof(pDescriptorHeaps), pDescriptorHeaps);
			m_Lists[i]->SetGraphicsRootDescriptorTable(0, pScene->m_RCBOHandle);
		}

		for (auto e : pScene->m_Obj) {
			this->Draw(e);
		}
	}

	void Renderer::Draw(RigidBody* pBody)
	{
		int effect = pBody->m_nEffect;
		if (effect < 0 || effect > m_nEffectCount) return;
		UINT count = pBody->m_VertexBuffers.size();

		m_Lists[effect]->SetGraphicsRootDescriptorTable(1, pBody->m_Handle);

		for (int i = 0; i < count; ++i) {
			m_Lists[effect]->IASetVertexBuffers(0, 1, &pBody->m_VertexBuffers[i].second);
			m_Lists[effect]->DrawInstanced(pBody->m_VertexBuffers[i].second.SizeInBytes / pBody->m_VertexBuffers[i].second.StrideInBytes, 1, 0, 0);
		}
	}

	void Renderer::m_ResizeSceneHeap(engine::Scene* pScene)
	{

		HRESULT hr = S_OK;

		pScene->m_ConstantHeap.Reset();
		for (auto e : m_Effects) e.m_Signature.Reset();

		D3D12_DESCRIPTOR_RANGE1 ranges[2];
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		ranges[0].NumDescriptors = 1;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = 0;
		ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		ranges[1].BaseShaderRegister = 1;
		ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		ranges[1].NumDescriptors = 1;
		ranges[1].RegisterSpace = 0;
		ranges[1].OffsetInDescriptorsFromTableStart = 0;
		ranges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		D3D12_ROOT_PARAMETER1 rootParameters[2];
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];

		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[1].DescriptorTable.pDescriptorRanges = &ranges[1];

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSignatureDesc.Desc_1_1.Flags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSignatureDesc.Desc_1_1.NumParameters = 2;
		rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
		rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

		ComPtr<ID3DBlob> error;
		ComPtr<ID3DBlob> signature;

		hr = (D3D12SerializeVersionedRootSignature(&rootSignatureDesc,
			&signature, &error)) throw_if_err;

		for (auto e : m_Effects) {

			hr = m_Device->CreateRootSignature(
				0,
				signature->GetBufferPointer(),
				signature->GetBufferSize(),
				IID_PPV_ARGS(&e.m_Signature)
			);
			throw_if_err;
		}

		D3D12_HEAP_PROPERTIES heapProps;
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = pScene->GetObjects().size() + 1;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		hr = (m_Device->CreateDescriptorHeap(&heapDesc,
			IID_PPV_ARGS(&pScene->m_ConstantHeap))) throw_if_err;

		//Create rcbo and setup
		if (!pScene->m_RCBOBuffer.Get()) {
			D3D12_HEAP_PROPERTIES heapProps;
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProps.CreationNodeMask = 1;
			heapProps.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC cbResourceDesc;
			cbResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			cbResourceDesc.Alignment = 0;
			cbResourceDesc.Width = (sizeof(engine::SceneShaderData) + 255) & ~255;
			cbResourceDesc.Height = 1;
			cbResourceDesc.DepthOrArraySize = 1;
			cbResourceDesc.MipLevels = 1;
			cbResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			cbResourceDesc.SampleDesc.Count = 1;
			cbResourceDesc.SampleDesc.Quality = 0;
			cbResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			cbResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			hr = (m_Device->CreateCommittedResource(
				&heapProps, D3D12_HEAP_FLAG_NONE, &cbResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&pScene->m_RCBOBuffer)));

			pScene->m_LocalShaderData.projection = glm::perspectiveFovLH<float>(45.f, m_ViewPort.Width, m_ViewPort.Height, 0.1, 1000);
			pScene->m_LocalShaderData.view = glm::mat4x4(1.f);
			pScene->m_LocalShaderData.pos = glm::vec4(0);

			m_UpdateCbuffer(pScene);

		}


		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = pScene->m_RCBOBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes =
				(sizeof(engine::SceneShaderData) + 255) & ~255;

			D3D12_CPU_DESCRIPTOR_HANDLE
				cbvHandle(pScene->m_ConstantHeap->GetCPUDescriptorHandleForHeapStart());
			cbvHandle.ptr = cbvHandle.ptr + m_Device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;

			m_Device->CreateConstantBufferView(&cbvDesc, cbvHandle);

			D3D12_GPU_DESCRIPTOR_HANDLE tHandle = (pScene->m_ConstantHeap->GetGPUDescriptorHandleForHeapStart());
			tHandle.ptr += m_Device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;
			pScene->m_RCBOHandle = tHandle;
		}

		int count = pScene->m_Obj.size() + 1;
		for (int u = 1; u < count; u++) {
			int i = u;
			RigidBody* pTemp = (RigidBody*)pScene->m_Obj.at(i - 1);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = pTemp->m_CBOBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes =
				(sizeof(engine::BodyShaderData) + 255) & ~255;

			D3D12_CPU_DESCRIPTOR_HANDLE
				cbvHandle(pScene->m_ConstantHeap->GetCPUDescriptorHandleForHeapStart());
			cbvHandle.ptr = cbvHandle.ptr + m_Device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) *
				i;

			m_Device->CreateConstantBufferView(&cbvDesc, cbvHandle);

			pTemp->m_CBONum = i;

			D3D12_GPU_DESCRIPTOR_HANDLE tHandle = (pScene->m_ConstantHeap->GetGPUDescriptorHandleForHeapStart());
			tHandle.ptr += m_Device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) *
				i;
			pTemp->m_Handle = tHandle;
		}

	}

	void Renderer::m_InitD3D12(HWND hWnd)
	{
		if (!hWnd) throw "HWND was NULL";

		HRESULT hr = S_OK;
		D3D_FEATURE_LEVEL minimumLevel = D3D_FEATURE_LEVEL_12_0;

		//Get client metrics
		{
			RECT clientRect;
			GetClientRect(hWnd, &clientRect);

			zmem(m_ViewPort);
			m_ViewPort.MaxDepth = 1.f;
			m_ViewPort.Width = clientRect.right - clientRect.left;
			m_ViewPort.Height = clientRect.bottom - clientRect.top;

			zmem(m_ScissorRect);
			m_ScissorRect.right = m_ViewPort.Width;
			m_ScissorRect.bottom = m_ViewPort.Height;
		}

		//Create DXGI factory
		{
			hr = CreateDXGIFactory(IID_PPV_ARGS(&m_Factory)) throw_if_err;
		}
		
		//Create debug
		{
#ifdef _DEBUG
			hr = D3D12GetDebugInterface(IID_PPV_ARGS(&m_Debug)) throw_if_err;
			m_Debug->EnableDebugLayer();
#endif // _DEBUG
		}

		//Get compatibility adapter
		{
			std::vector<IDXGIAdapter*> pEnumAdapters;
			IDXGIAdapter* pTempAdapter = nullptr;
			ID3D12Device* pTempDevice = nullptr;

			//Search all
			{
				for (int i = 0; ; ++i) {
					if (m_Factory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&pTempAdapter))
						== DXGI_ERROR_NOT_FOUND) break;
						pTempAdapter->AddRef();
						pEnumAdapters.push_back(pTempAdapter);
						pTempAdapter->Release();
						pTempAdapter = nullptr;
					
				}
			}

			//Choose one
			{
				for (auto adapter : pEnumAdapters) {
					HRESULT tempHR = D3D12CreateDevice(adapter, minimumLevel, IID_PPV_ARGS(&pTempDevice));
					if(SUCCEEDED(tempHR)) {
						D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
						pTempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
						if (options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
							pTempDevice->Release();
							pTempAdapter = adapter;
							adapter->AddRef();
							break;
						}
					}
					pTempDevice->Release();
				}

				if (!pTempAdapter) throw_err("compatibility adapter not found");

				pTempAdapter->QueryInterface(IID_PPV_ARGS(&m_Adapter));
				pTempAdapter->Release();

				for (auto adapter : pEnumAdapters) adapter->Release();
			}

		}

		//Create device and debug device
		{
			hr = D3D12CreateDevice(m_Adapter.Get(), minimumLevel, IID_PPV_ARGS(&m_Device)) throw_if_err;
#ifdef _DEBUG
			hr = m_Device->QueryInterface(IID_PPV_ARGS(&m_DebugDevice)) throw_if_err;
#endif // _DEBUG
		}

		//Create command queue
		{
			D3D12_COMMAND_QUEUE_DESC queueDesc;
			zmem(queueDesc);
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			hr = m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_Queue)) throw_if_err;
		}

		//Create swap chain
		{
			//create swap chain
			{
				DXGI_SWAP_CHAIN_DESC1 ds;
				zmem(ds);
				ds.BufferCount = m_nBufferCount;
				ds.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				ds.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				ds.Height = m_ViewPort.Height;
				ds.Width = m_ViewPort.Width;
				ds.SampleDesc.Count = 1;
				ds.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

				DXGI_SWAP_CHAIN_FULLSCREEN_DESC fds;
				zmem(fds);
				fds.Windowed = TRUE;

				IDXGISwapChain1* pTempSC = nullptr;
				hr = m_Factory->CreateSwapChainForHwnd(m_Queue.Get(), hWnd, &ds, &fds, nullptr, &pTempSC) throw_if_err;
				pTempSC->QueryInterface(IID_PPV_ARGS(&m_SwapChain));
				srel(pTempSC);
			}

			//Create rtv desc heap
			{
				D3D12_DESCRIPTOR_HEAP_DESC ds;
				zmem(ds);
				ds.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				ds.NumDescriptors = m_nBufferCount;
				hr = m_Device->CreateDescriptorHeap(&ds, IID_PPV_ARGS(&m_RTVHeap)) throw_if_err;
				m_nRTVDescSize = m_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			}
			//Create rtv`s
			{
				ID3D12Resource* pTempRes = nullptr;
				CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart());
				for (int i = 0; i < m_nBufferCount; ++i) {
					hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&pTempRes)) throw_if_err;
					m_Device->CreateRenderTargetView(pTempRes, nullptr, descHandle);
					pTempRes->AddRef();
					m_RTVs.push_back(pTempRes);
					pTempRes->Release();
					descHandle.Offset(m_nRTVDescSize);
				}
			}

		}

		m_InitPipeLine(hWnd);

		//Create sync objects
		{
			hr = m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence)) throw_if_err;
			m_FenceValue = 1;

			m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (m_FenceEvent == nullptr)
			{
				hr = HRESULT_FROM_WIN32(GetLastError()) throw_if_err;
			}

			m_Sync();
		}

	}

	void Renderer::m_InitPipeLine(HWND hWnd)
	{
		HRESULT hr = S_OK;
		
		std::function<void(UINT, ID3DBlob*, ID3DBlob*, D3D12_INPUT_ELEMENT_DESC*)> CreateEffect;

		//Predefined lambda
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC pds;
			zmem(pds);
			CreateEffect = [&](UINT index, ID3DBlob *pVS, ID3DBlob* pPS, D3D12_INPUT_ELEMENT_DESC *ied) -> void {
				m_Effects.resize(m_nEffectCount + 1);
				m_Lists.resize(m_nEffectCount + 1);
				m_Allocators.resize(m_nEffectCount + 1);
				D3D12_INPUT_ELEMENT_DESC inputElementDesc[] =
				{
					{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
					{"NORMAL",	0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
				};

				{
					D3D12_DESCRIPTOR_RANGE1 ranges[2];
					ranges[0].BaseShaderRegister = 0;
					ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
					ranges[0].NumDescriptors = 1;
					ranges[0].RegisterSpace = 0;
					ranges[0].OffsetInDescriptorsFromTableStart = 0;
					ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

					ranges[1].BaseShaderRegister = 1;
					ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
					ranges[1].NumDescriptors = 1;
					ranges[1].RegisterSpace = 0;
					ranges[1].OffsetInDescriptorsFromTableStart = 0;
					ranges[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

					D3D12_ROOT_PARAMETER1 rootParameters[2];
					rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

					rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
					rootParameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];

					rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
					rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

					rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
					rootParameters[1].DescriptorTable.pDescriptorRanges = &ranges[1];

					D3D12_VERSIONED_ROOT_SIGNATURE_DESC ds;
					zmem(ds);
					ds.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
					ds.Desc_1_1.Flags =
						D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
					ds.Desc_1_1.NumParameters = 2;
					ds.Desc_1_1.pParameters = rootParameters;
					ds.Desc_1_1.NumStaticSamplers = 0;
					ds.Desc_1_1.pStaticSamplers = nullptr;
					ID3DBlob* pSignature;
					hr = D3D12SerializeVersionedRootSignature(&ds, &pSignature, nullptr)
						throw_if_err;
					hr = m_Device->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&(m_Effects.at(index).m_Signature))) throw_if_err;
				}

					zmem(pds);
					pds.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
					pds.InputLayout.NumElements = 2;
					pds.InputLayout.pInputElementDescs = inputElementDesc;
					pds.pRootSignature = (m_Effects.at(index).m_Signature).Get();

					pds.VS = CD3DX12_SHADER_BYTECODE(pVS);
					pds.PS = CD3DX12_SHADER_BYTECODE(pPS);

					pds.SampleMask = UINT_MAX;
					pds.SampleDesc.Count = 1;
					pds.SampleDesc.Quality = 0;
					pds.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
					pds.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
					pds.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

					pds.NumRenderTargets = 1;
					pds.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
					
					hr = m_Device->CreateGraphicsPipelineState(&pds, IID_PPV_ARGS(&(m_Effects.at(index).m_PipelineState))) throw_if_err;

					//Create command allocator
					{
						hr = m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_Allocators.at(index))) throw_if_err;
					}

					//Create command list
					{
						hr = m_Device->CreateCommandList(NULL, D3D12_COMMAND_LIST_TYPE_DIRECT, m_Allocators.at(index).Get(), m_Effects.at(index).m_PipelineState.Get(), IID_PPV_ARGS(&m_Lists.at(index))) throw_if_err;
						m_Lists.at(index)->Close();
					}

					m_nEffectCount++;

				};
		}
		

		////Create effect 0(BASE)
		//{
		//	auto shaders = g_LoadShaders(L"Data/FX/base");
		//	CreateEffect(0, shaders.first, shaders.second, nullptr);
		//	srel(shaders.first);
		//	srel(shaders.second);
		//}
		//
		////Create effect 1
		//{
		//	auto shaders = g_LoadShaders(L"Data/FX/test");
		//	CreateEffect(1, shaders.first, shaders.second, nullptr);
		//	srel(shaders.first);
		//	srel(shaders.second);
		//}

		//Create effect 2(light)
		{
			auto shaders = g_LoadShaders(L"Data/FX/light");
			CreateEffect(0, shaders.first, shaders.second, nullptr);
			srel(shaders.first);
			srel(shaders.second);
		}

	}

	void Renderer::m_Sync()
	{

		HRESULT hr = S_OK;
		const UINT64 fence = m_FenceValue;
		hr = m_Queue->Signal(m_Fence.Get(), fence) throw_if_err;
		m_FenceValue++;

		if (m_Fence->GetCompletedValue() < fence)
		{
			hr = m_Fence->SetEventOnCompletion(fence, m_FenceEvent) throw_if_err;
			WaitForSingleObject(m_FenceEvent, INFINITE);
		}

		m_nRTVIndex = m_SwapChain->GetCurrentBackBufferIndex();

	}

	void Renderer::m_UpdateCbuffer(engine::Scene* pScene)
	{

		auto update = [&](ID3D12Resource* pRes, void * data, UINT size) -> void {
			D3D12_RANGE readRange;
			readRange.Begin = 0;
			readRange.End = 0;

			UINT8* pMapped;

			(pRes->Map(
				0, &readRange, reinterpret_cast<void**>(&pMapped)));
			memcpy(pMapped, data, size);
			pRes->Unmap(0, &readRange);
		};

		if (pScene->m_NeedHeapUpdate) {
			m_ResizeSceneHeap(pScene);
			pScene->m_NeedHeapUpdate = 0;
		}

		
		for (auto e : pScene->m_Obj) {
			if (e->m_NeedUpdate) {
				e->m_Update();
				update(e->m_CBOBuffer.Get(), &e->m_LocalShaderData, sizeof(engine::BodyShaderData));
				e->m_NeedUpdate = 0;
			}
		}

		if (pScene->m_NeedConstantUpdate) {
			update(pScene->m_RCBOBuffer.Get(), &pScene->m_LocalShaderData, sizeof(engine::SceneShaderData));
			pScene->m_NeedConstantUpdate = 0;
		}
	

	}

	std::pair<ID3DBlob*, ID3DBlob*> g_LoadShaders(LPCWSTR pcwFileName) {

		HRESULT hr = S_OK;
		ID3DBlob* pVS, *pPS, *pERR;

		//Load vertex
		{
			hr = D3DCompileFromFile((std::wstring(pcwFileName) + (L".evs")).c_str(), nullptr, nullptr, "main", "vs_5_0", NULL, NULL, &pVS, &pERR);
			if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) { if(pERR->GetBufferSize() > 16) MessageBoxA(NULL, (char*)pERR->GetBufferPointer(), "D3DCompile Error", MB_OK); pVS = nullptr;
			}
		}

		//Load pixel
		{
			hr = D3DCompileFromFile((std::wstring(pcwFileName) + (L".eps")).c_str(), nullptr, nullptr, "main", "ps_5_0", NULL, NULL, &pPS, &pERR);
			if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) { if (pERR->GetBufferSize() > 16) MessageBoxA(NULL, (char*)pERR->GetBufferPointer(), "D3DCompile Error", MB_OK); pPS = nullptr;
			}
		}

		return std::make_pair(pVS, pPS);
	}

}