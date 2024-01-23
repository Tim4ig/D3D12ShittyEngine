
#pragma once

#include "MacroDef.hpp"

#include <D3DX12/d3dx12.h>
#include <dxgi1_6.h>

#include "Scene.hpp"

namespace engine {

	struct Effect {
		ComPtr<ID3D12PipelineState>			m_PipelineState;
		ComPtr<ID3D12RootSignature>			m_Signature;
	};

	class Renderer final {

	public:

		static Renderer*	CreateRendererInstance(HWND hWnd);
		static Renderer*	GetRendererInstance();
		static void			DeleteRendererInstance();

		void Begin();
		void Clear();
		void End();

		ID3D12Device5* GetDevice();

		void DrawScene();
		void Draw(RigidBody * pBody);

		~Renderer() = default;

	public:

		Renderer() = default;

		void m_ResizeSceneHeap(engine::Scene* pScene);

		void m_InitD3D12(HWND hWnd);
		void m_InitPipeLine(HWND hWnd);
		void m_InitRes();
		void m_Sync();

		void m_UpdateCbuffer(engine::Scene *pScene);

		const int						m_nBufferCount = 2;
		UINT							m_nEffectCount = 0;

		/***********************************************************************/
		ComPtr<IDXGIFactory6>				m_Factory;
		ComPtr<IDXGIAdapter>				m_Adapter;

		ComPtr<ID3D12Device5>				m_Device;
		ComPtr<ID3D12Debug>					m_Debug;
		ComPtr<ID3D12DebugDevice>			m_DebugDevice;
		
		ComPtr<IDXGISwapChain3>				m_SwapChain;
		ComPtr<ID3D12DescriptorHeap>		m_RTVHeap;
		std::vector<ComPtr<ID3D12Resource>> m_RTVs;
		UINT								m_nRTVDescSize = 0;
		UINT								m_nRTVIndex = 0;

		ComPtr<ID3D12CommandQueue>			m_Queue;

		HANDLE								m_FenceEvent = 0;
		ComPtr<ID3D12Fence>					m_Fence;
		UINT64								m_FenceValue = 0;

		//std::vector<ComPtr<ID3D12Resource>>	m_Res;

		std::vector <Effect>				m_Effects;
		std::vector <ComPtr<ID3D12GraphicsCommandList>> m_Lists;
		std::vector <ComPtr<ID3D12CommandAllocator>> m_Allocators;

		/***********************************************************************/
		D3D12_VIEWPORT					m_ViewPort;
		D3D12_RECT						m_ScissorRect;


	};

}