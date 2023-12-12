#pragma once
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <memory>
#include <string>
#include "DirectXMath.h"
#include <d3dcompiler.h>
#include "CommandQueue.h"
#include "ErrorHelpers.h"
#include <chrono>
using namespace Microsoft::WRL;
using namespace DirectX;

// The min/max macros conflict with like-named member functions.
// Only use std::min and std::max defined in <algorithm>.
#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif


class Application
{
public:
	Application(const Application&) = delete;
	Application& operator=(Application&) = delete;
	static Application& GetInstance() {
		static Application instance;
		return instance;
	}
	void SetHWND(HWND hwnd) {
		m_hWnd = hwnd;
	}
	void Initiate();
	void Update();
	void Destroy();
protected:

	/* The number of swap chain back buffers*/
	static const UINT16 FrameCount = 3;
	/*Depending on the flip model of the swap chain,
	the index of the current back buffer in the swap chain may not be sequential.*/
	UINT                              CurrentBackBufferIndex = NULL;
	/*In order to correctly offset the index into the descriptor heap,
	the size of a single element in the descriptor heap needs to be queried during initialization*/
	bool isTearingSupported		      = false;
	bool g_isInitialized = false;
	UINT                              rtvDescriptorSize;
	ComPtr<IDXGIFactory7>			  dxgiFactory;
	ComPtr<ID3D12Device2>			  d3d12Device;
	ComPtr<IDXGIAdapter4>			  dxgiAdapter;
	ComPtr<ID3D12Resource>            BackBuffers[FrameCount];
	/*There must be at least one command allocator per render frame*/
	ComPtr<ID3D12CommandAllocator>    CommandAllocators[FrameCount];
	ComPtr<ID3D12CommandQueue>        CommandQueue;
	ComPtr<ID3D12RootSignature>       RootSignature;
	/*A descriptor heap can be visualized as an array of descriptors (views).
	A view simply describes a resource that resides in GPU memory.*/
	ComPtr<ID3D12DescriptorHeap>      rtvHeap;
	ComPtr<ID3D12DescriptorHeap>      dsvHeap;
	ComPtr<ID3D12DescriptorHeap>      srvHeap;
	ComPtr<ID3D12PipelineState>       PipelineState;
	ComPtr<ID3D12GraphicsCommandList> CommandList;
	ComPtr<IDXGISwapChain4>           SwapChain;
	HWND							  m_hWnd = NULL;
	//                                Synchronization objects
	ComPtr<ID3D12Fence>               g_fence;
	UINT64                            g_fenceValue = NULL;
	/*Used to keep track of the fence values that were used to signal the command queue for a particular frame.*/
	UINT64                            g_frameFenceValues[FrameCount] = {};
	/*A handle to an OS event object that will be used to receive the notification that the fence has reached a specific value.*/
	HANDLE                            g_fenceEvent;
	// App resources.
	/* Destination resource for the vertex buffer data and is used for rendering the cube geometry.*/
	ComPtr<ID3D12Resource>            g_vertexBuffer;
	ComPtr<ID3D12Resource>            g_indexBuffer;
	ComPtr<ID3D12Resource>            g_depthBuffer;
	/* Used to tell the Input Assembler stage where the vertices are stored in GPU memory.*/
	D3D12_VERTEX_BUFFER_VIEW          g_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW           g_indexBufferView;
private:
	Application(){}
	void InitiateDebugLayer();
	ComPtr<IDXGIAdapter4>   GetAdapter(bool useWarp);
	ComPtr<ID3D12Device2>   MakeDevice(ComPtr<IDXGIAdapter4> adapter);
	ComPtr<IDXGISwapChain4> MakeSwapChain(HWND hwnd, ComPtr<ID3D12CommandQueue> commandQueue, UINT32 width, UINT32 height, UINT32 bufferCount);
	bool CheckTearingSupport();
	ComPtr<ID3D12DescriptorHeap> MakeDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT32 numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
	ComPtr<ID3D12CommandQueue> MakeCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
	ComPtr<ID3D12CommandAllocator> MakeCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	ComPtr<ID3D12GraphicsCommandList> MakeCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);
	ComPtr<ID3D12Fence> MakeFence(ComPtr<ID3D12Device2> device);
	HANDLE MakeEventHandle();
	void UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList> commandList, ID3D12Resource** pDestinationResource,
		ID3D12Resource** pIntermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
	void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
	void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, UINT64& fenceValue, HANDLE fenceEvent);
	UINT64 Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, UINT64& fenceValue);
	void WaitForFenceValue(ComPtr<ID3D12Fence> fence, UINT64 fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
	void ResizeDepthBuffer(int width, int height);
};