
#include "Renderer.hpp"

#include <vector>
#include <functional>

#include <d3dcompiler.h>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#define perps glm::perspectiveFovLH<float>(45.f, m_ViewPort.Width, m_ViewPort.Height, 0.1f, 1000.f)

namespace engine {

	Renderer* g_pRendererInstance = nullptr;
	std::pair<ID3DBlob*, ID3DBlob*> g_LoadShaders(LPCWSTR pcwFileName);

	Renderer* Renderer::CreateRendererInstance(HWND hWnd) noexcept(false)
	{
		//Create and init renderer if need
		if (g_pRendererInstance) return g_pRendererInstance;
		g_pRendererInstance = new Renderer;
		g_pRendererInstance->m_InitD3D12(hWnd);
		return g_pRendererInstance;
	}

	Renderer* Renderer::GetRendererInstance() noexcept
	{
		return g_pRendererInstance;
	}

	void Renderer::DeleteRendererInstance() noexcept
	{
		delete g_pRendererInstance;
	}

	void Renderer::Begin() noexcept
	{
		//Reset all command allocators
		for(auto & allocator : m_Allocators) allocator->Reset();
		
		//Prepare the buffer for drawing
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_RTBs[m_nRTVIndex].Get(),
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDesc(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), m_nRTVIndex, m_nRTVDescSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDesc(m_DSVHeap->GetCPUDescriptorHandleForHeapStart());

		//Setup command lists
		for (UINT i = 0; i < m_nEffectCount; ++i)
		{
			m_Lists[i]->Reset(m_Allocators[i].Get(), m_Effects[i].m_PipelineState.Get());
			m_Lists[i]->SetGraphicsRootSignature(m_Effects[i].m_Signature.Get());
			m_Lists[i]->ResourceBarrier(1, &barrier);
			m_Lists[i]->RSSetViewports(1, &m_ViewPort);
			m_Lists[i]->RSSetScissorRects(1, &m_ScissorRect);
			m_Lists[i]->OMSetRenderTargets(1, &rtvDesc, FALSE, &dsvDesc);
			m_Lists[i]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}
	}

	void Renderer::Clear() noexcept
	{
		//Get current back-buffer
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvDesc(m_RTVHeap->GetCPUDescriptorHandleForHeapStart(), m_nRTVIndex, m_nRTVDescSize);
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvDesc(m_DSVHeap->GetCPUDescriptorHandleForHeapStart());

		//Black color as default
		int ClearColor = 0;
		Scene* pScene = GetActiveScene();

		//If there is an active scene, get the color of the scene
		if (pScene) ClearColor = pScene->m_ClearColor;

		//Parse and clear
		float r = ((ClearColor >> 24) & 255) / 255.f;
		float g = ((ClearColor >> 16) & 255) / 255.f;
		float b = ((ClearColor >> 8) & 255) / 255.f;
		float a = ((ClearColor >> 0) & 255) / 255.f;
		float clr[] = { r,g,b,a };
		m_Lists.at(0)->ClearRenderTargetView(rtvDesc, clr, 0, nullptr);
		m_Lists.at(0)->ClearDepthStencilView(dsvDesc, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, NULL);
	}

	void Renderer::End() noexcept
	{
		//Prepare the buffer for showing
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			m_RTBs[m_nRTVIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);

		for (auto & eff : m_Lists)
		{
			eff->ResourceBarrier(1, &barrier);
			eff->Close();
		}
	
		ID3D12CommandList** ppLists = reinterpret_cast<ID3D12CommandList**>(m_Lists[0].GetAddressOf());

		m_Queue->ExecuteCommandLists(static_cast<UINT>(m_Effects.size()), ppLists);
		m_SwapChain->Present(1, 0);

		m_Sync();
	}

	ID3D12Device5* Renderer::GetDevice() noexcept
	{
		return m_Device.Get();
	}

	void Renderer::DrawScene() noexcept
	{
		//Get active scene
		engine::Scene* pScene = engine::GetActiveScene();
		if (!pScene) return;

		//Update scene cbuffers
		m_UpdateCbuffer(pScene);
		
		//Setup heaps
		for (int i = 0; i < m_Effects.size(); ++i) {
			ID3D12DescriptorHeap* pDescriptorHeaps[] = { pScene->m_ConstantHeap.Get() };
			m_Lists[i]->SetDescriptorHeaps(_countof(pDescriptorHeaps), pDescriptorHeaps);
			m_Lists[i]->SetGraphicsRootDescriptorTable(0, pScene->m_RCBOHandle);
		}

		//Draw all obj
		for (auto & e : pScene->m_Obj) {
			this->Draw(e);
		}
	}

	void Renderer::Draw(RigidBody* pBody) noexcept
	{
		//Get obj data
		UINT effect = pBody->m_nEffect;
		UINT count = static_cast<UINT>(pBody->m_VertexBuffers.size());
		if (effect < 0 || effect > m_nEffectCount) return;
		
		//Set cbuffer
		m_Lists[effect]->SetGraphicsRootDescriptorTable(1, pBody->m_Handle);

		//Draw all buffers
		for (UINT i = 0; i < count; ++i) {
			m_Lists[effect]->IASetVertexBuffers(0, 1, &pBody->m_VertexBuffers[i].second);
			m_Lists[effect]->DrawInstanced(pBody->m_VertexBuffers[i].second.SizeInBytes / pBody->m_VertexBuffers[i].second.StrideInBytes, 1, 0, 0);
		}
	}

	void Renderer::m_RecreateRootSignature() noexcept(false)
	{
		HRESULT hr = S_OK;

		for (auto& e : m_Effects) e.m_Signature.Reset();

		//Setup root data
		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		{
			D3D12_DESCRIPTOR_RANGE1 ranges[2];
			memset(ranges, 0, sizeof(D3D12_DESCRIPTOR_RANGE1) * __crt_countof(ranges));
			ranges[0].BaseShaderRegister = 0;
			ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			ranges[0].NumDescriptors = 1;
			ranges[0].RegisterSpace = 0;
			ranges[0].OffsetInDescriptorsFromTableStart = 0;
			ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

			ranges[1] = ranges[0];
			ranges[1].BaseShaderRegister = 1;

			D3D12_ROOT_PARAMETER1 rootParameters[2];
			memset(rootParameters, 0, sizeof(D3D12_ROOT_PARAMETER1) * __crt_countof(rootParameters));
			rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

			rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
			rootParameters[0].DescriptorTable.pDescriptorRanges = &ranges[0];

			rootParameters[1] = rootParameters[0];
			rootParameters[1].DescriptorTable.pDescriptorRanges = &ranges[1];

			zmem(rootSignatureDesc);
			rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
			rootSignatureDesc.Desc_1_1.Flags =
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			rootSignatureDesc.Desc_1_1.NumParameters = 2;
			rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
			rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
			rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;
		}

		//Serialize
		ComPtr<ID3DBlob> error;
		ComPtr<ID3DBlob> signature;
		hr = (D3D12SerializeVersionedRootSignature(&rootSignatureDesc,
			&signature, &error)) throw_if_err;

		//Create signature for all effects
		for (auto& e : m_Effects) {

			hr = m_Device->CreateRootSignature(
				0,
				signature->GetBufferPointer(),
				signature->GetBufferSize(),
				IID_PPV_ARGS(&e.m_Signature)
			);
			throw_if_err;
		}

	}

	void Renderer::m_InitD3D12(HWND hWnd) noexcept(false)
	{
		if (!hWnd) throw "HWND was NULL";

		HRESULT hr = S_OK;
		D3D_FEATURE_LEVEL minimumLevel = D3D_FEATURE_LEVEL_12_0;
		m_hTargetWindow = hWnd;

		//Get client metrics
		{
			RECT clientRect;
			GetClientRect(hWnd, &clientRect);

			zmem(m_ViewPort);
			m_ViewPort.MaxDepth = 1.f;
			m_ViewPort.Width =  static_cast<FLOAT>(clientRect.right - clientRect.left);
			m_ViewPort.Height = static_cast<FLOAT>(clientRect.bottom - clientRect.top);

			zmem(m_ScissorRect);
			m_ScissorRect.right =	static_cast<LONG>(m_ViewPort.Width	);
			m_ScissorRect.bottom =	static_cast<LONG>(m_ViewPort.Height);
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

			hr = DXGIGetDebugInterface1(NULL, IID_PPV_ARGS(&m_DXGIDebug)) throw_if_err;
			m_DXGIDebug->EnableLeakTrackingForThread();
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
				for (auto & adapter : pEnumAdapters) {
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

				for (auto & adapter : pEnumAdapters) adapter->Release();
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
				ds.Height = static_cast<UINT>(m_ViewPort.Height);
				ds.Width =	static_cast<UINT>(m_ViewPort.Width );
				ds.SampleDesc.Count = 1;
				ds.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

				DXGI_SWAP_CHAIN_FULLSCREEN_DESC fds;
				zmem(fds);
				fds.Windowed = TRUE;

				IDXGISwapChain1* pTempSC = nullptr;
				hr = m_Factory->CreateSwapChainForHwnd(m_Queue.Get(), hWnd, &ds, &fds, nullptr, &pTempSC) throw_if_err;
				pTempSC->QueryInterface(IID_PPV_ARGS(&m_SwapChain));
				pTempSC->Release();
				
			}

			m_InitSwapChainBuffers();

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

	void Renderer::m_InitPipeLine(HWND hWnd) noexcept(false)
	{
		HRESULT hr = S_OK;
		
		std::function<void(UINT, ID3DBlob*, ID3DBlob*, D3D12_INPUT_ELEMENT_DESC*)> CreateEffect;

		//Predefined lambda
		{
			D3D12_GRAPHICS_PIPELINE_STATE_DESC pds;
			zmem(pds);
			CreateEffect = [&](UINT index, ID3DBlob* pVS, ID3DBlob* pPS, D3D12_INPUT_ELEMENT_DESC* ied) -> void {
				m_Effects.resize(m_nEffectCount + 1);
				m_Lists.resize(m_nEffectCount + 1);
				m_Allocators.resize(m_nEffectCount + 1);
				D3D12_INPUT_ELEMENT_DESC inputElementDesc[] =
				{
					{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
					{"NORMAL",	0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
				};

				m_RecreateRootSignature();

				//Create pipeline state
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
				pds.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
				pds.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
				pds.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
				pds.NumRenderTargets = 1;
				pds.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

				D3D12_DEPTH_STENCIL_DESC depthDesc = {};
				depthDesc.DepthEnable = TRUE;
				depthDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
				depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

				pds.DepthStencilState = depthDesc;
				pds.DSVFormat = DXGI_FORMAT_D32_FLOAT;

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

		//Create effect 0(light)
		{
			auto shaders = g_LoadShaders(L"Data/FX/light");
			CreateEffect(0, shaders.first, shaders.second, nullptr);
			srel(shaders.first);
			srel(shaders.second);
		}
	}

	void Renderer::m_Sync() noexcept(false)
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

	void Renderer::Resize() noexcept(false)
	{
		HRESULT hr = S_OK;
		//Get new client metrics
		{
			RECT clientRect;
			GetClientRect(m_hTargetWindow, &clientRect);

			if (clientRect.right - clientRect.left < 10 || clientRect.bottom - clientRect.top < 10) return;

			zmem(m_ViewPort);
			m_ViewPort.MaxDepth = 1.f;
			m_ViewPort.Width = static_cast<FLOAT>(clientRect.right - clientRect.left);
			m_ViewPort.Height = static_cast<FLOAT>(clientRect.bottom - clientRect.top);

			zmem(m_ScissorRect);
			m_ScissorRect.right = static_cast<LONG>(m_ViewPort.Width);
			m_ScissorRect.bottom = static_cast<LONG>(m_ViewPort.Height);


		}
		//Release old buffers
		{
			m_RTBs.clear();
			m_DSB.Reset();
			m_RTVHeap.Reset();
			m_DSVHeap.Reset();
		}
		//Resize swapChainBuffers
		hr = m_SwapChain->ResizeBuffers(
			m_nBufferCount,
			static_cast<UINT>(m_ViewPort.Width),
			static_cast<UINT>(m_ViewPort.Height),
			DXGI_FORMAT_R8G8B8A8_UNORM,
			NULL
		) throw_if_err;
		//Create new buffers
		m_InitSwapChainBuffers();
		m_Sync();
		if (GetActiveScene()) {
			GetActiveScene()->m_LocalShaderData.projection = perps;
			GetActiveScene()->m_NeedConstantUpdate = 1;
		}
	}

	void Renderer::m_InitSwapChainBuffers() noexcept(false)
	{
		HRESULT hr = S_OK;
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
			
			CD3DX12_CPU_DESCRIPTOR_HANDLE descHandle(m_RTVHeap->GetCPUDescriptorHandleForHeapStart());
			for (int i = 0; i < m_nBufferCount; ++i) {
				ID3D12Resource* pTempRes = nullptr;
				hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&pTempRes)) throw_if_err;
				m_Device->CreateRenderTargetView(pTempRes, nullptr, descHandle);
				m_RTBs.push_back(pTempRes);
				pTempRes->Release();
				descHandle.Offset(m_nRTVDescSize);
			}
		}

		//Create dsv
		{
			//Heap
			{
				D3D12_DESCRIPTOR_HEAP_DESC ds = {};
				ds.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
				ds.NumDescriptors = 1;

				hr = m_Device->CreateDescriptorHeap(&ds, IID_PPV_ARGS(&m_DSVHeap)) throw_if_err;
			}
			//Buffer
			{
				D3D12_RESOURCE_DESC depthStencilDesc = {};
				depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				depthStencilDesc.Alignment = 0;
				depthStencilDesc.Width =	static_cast<UINT64>(m_ViewPort.Width );
				depthStencilDesc.Height =	static_cast<UINT64>(m_ViewPort.Height);
				depthStencilDesc.DepthOrArraySize = 1;
				depthStencilDesc.MipLevels = 1;
				depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
				depthStencilDesc.SampleDesc.Count = 1;
				depthStencilDesc.SampleDesc.Quality = 0;
				depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
				depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

				D3D12_CLEAR_VALUE clearValue = {};
				clearValue.Format = DXGI_FORMAT_D32_FLOAT;
				clearValue.DepthStencil.Depth = 1.0f;
				clearValue.DepthStencil.Stencil = 0;

				auto properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

				hr = m_Device->CreateCommittedResource(
					&properties,
					D3D12_HEAP_FLAG_NONE,
					&depthStencilDesc,
					D3D12_RESOURCE_STATE_COMMON,
					&clearValue,
					IID_PPV_ARGS(&m_DSB)
				) throw_if_err;
			}

			D3D12_DEPTH_STENCIL_VIEW_DESC dsViewDesk = {};

			dsViewDesk.Format = DXGI_FORMAT_D32_FLOAT;
			dsViewDesk.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

			D3D12_CPU_DESCRIPTOR_HANDLE heapHandleDsv = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
			m_Device->CreateDepthStencilView(m_DSB.Get(), &dsViewDesk, heapHandleDsv);
		}
	}

	void Renderer::m_ResizeSceneHeap(engine::Scene* pScene) noexcept(false)
	{
		HRESULT hr = S_OK;

		//Delete old heap
		pScene->m_ConstantHeap.Reset();

		this->m_RecreateRootSignature();

		//Setup and create a new heap
		{
			D3D12_HEAP_PROPERTIES heapProps = {};
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProps.CreationNodeMask = 1;
			heapProps.VisibleNodeMask = 1;

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
			heapDesc.NumDescriptors = static_cast<UINT>(pScene->GetObjects().size() + 1);
			heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			hr = (m_Device->CreateDescriptorHeap(&heapDesc,
				IID_PPV_ARGS(&pScene->m_ConstantHeap))) throw_if_err;
		}

		//Create base cbo buffer
		if (!pScene->m_RCBOBuffer.Get()) {
			D3D12_HEAP_PROPERTIES heapProps = {};
			heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
			heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			heapProps.CreationNodeMask = 1;
			heapProps.VisibleNodeMask = 1;

			D3D12_RESOURCE_DESC cbResourceDesc = {};
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

			pScene->m_LocalShaderData.projection = perps;
			pScene->m_LocalShaderData.view = glm::mat4x4(1.f);
			pScene->m_LocalShaderData.pos = glm::vec4(0);
		}

		//Create base cbo buffer view
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = pScene->m_RCBOBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes =
				(sizeof(engine::SceneShaderData) + 255) & ~255;

			D3D12_CPU_DESCRIPTOR_HANDLE
				cbvHandle(pScene->m_ConstantHeap->GetCPUDescriptorHandleForHeapStart());
			cbvHandle.ptr = cbvHandle.ptr + static_cast<SIZE_T>(m_Device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0);

			m_Device->CreateConstantBufferView(&cbvDesc, cbvHandle);

			D3D12_GPU_DESCRIPTOR_HANDLE tHandle = (pScene->m_ConstantHeap->GetGPUDescriptorHandleForHeapStart());
			tHandle.ptr += static_cast<SIZE_T>(m_Device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0);
			pScene->m_RCBOHandle = tHandle;
		}

		//Create obje cbo buffer view`s
		int count = static_cast<int>(pScene->m_Obj.size() + 1);
		for (int u = 1; u < count; u++) {
			RigidBody* pTemp = (RigidBody*)pScene->m_Obj.at(u - 1);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = pTemp->m_CBOBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes =
				(sizeof(engine::BodyShaderData) + 255) & ~255;

			D3D12_CPU_DESCRIPTOR_HANDLE
				cbvHandle(pScene->m_ConstantHeap->GetCPUDescriptorHandleForHeapStart());
			cbvHandle.ptr += static_cast<SIZE_T>(m_Device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) *
				u);

			m_Device->CreateConstantBufferView(&cbvDesc, cbvHandle);

			D3D12_GPU_DESCRIPTOR_HANDLE tHandle = (pScene->m_ConstantHeap->GetGPUDescriptorHandleForHeapStart());
			tHandle.ptr += static_cast<SIZE_T>(m_Device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) *
				u);
			pTemp->m_Handle = tHandle;
		}
	}

	void Renderer::m_UpdateCbuffer(engine::Scene* pScene) noexcept(false)
	{
		auto update = [&](ID3D12Resource* pRes, void * data, UINT size) -> void {
			D3D12_RANGE readRange = {};
			readRange.Begin = 0;
			readRange.End = 0;

			UINT8* pMapped = nullptr;

			(pRes->Map(
				0, &readRange, reinterpret_cast<void**>(&pMapped)));
			memcpy(pMapped, data, size);
			pRes->Unmap(0, &readRange);
		};

		if (pScene->m_NeedHeapUpdate) {
			m_ResizeSceneHeap(pScene);
			pScene->m_NeedHeapUpdate = 0;
		}

		for (auto & e : pScene->m_Obj) {
			if (e->m_NeedUpdate) {
				e->m_Update();
				update(e->m_CBOBuffer.Get(), &e->m_LocalShaderData, sizeof(engine::BodyShaderData));
				e->m_NeedUpdate = 0;
			}
		}

		if (pScene->GetCamera()->m_bNeedUpdate) {

			pScene->m_LocalShaderData.view = glm::lookAtLH(
				pScene->GetCamera()->m_Pos,
				pScene->GetCamera()->m_Pos - pScene->GetCamera()->m_Target,
				glm::vec3(0, 1, 0)
			);

			pScene->GetCamera()->m_bNeedUpdate = false;
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