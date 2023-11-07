#include "pch.hpp"
#define MAX_NAME_STRING 256
using namespace DirectX;
using namespace Microsoft::WRL;

/*------------------*/
/* Global Variables */
/*------------------*/
#pragma region Global Variables
//            Window Settings.
const UINT16  CLIENT_WIDTH = 640;
const UINT16  CLIENT_HEIGHT = 480;
HICON         hIcon;
WCHAR         windowTitle[MAX_NAME_STRING];
WCHAR         className[MAX_NAME_STRING];
HWND          hwnd;
/*Window rectangle(used to toggle fullscreen state).
 used to store the previous window dimensions before going to fullscreen mode.*/
RECT g_WindowRect;

//                                Pipeline Objects.

/* The number of swap chain back buffers*/
static const UINT16 
g_frameCount                        = 2;
/* Use WARP adapter*/
bool g_useWarp                    = false;
/*Set to true once the DX12 objects have been initialized.*/ 
bool g_IsInitialized              = false;
bool g_VSync                      = true;
bool g_TearingSupported           = false;
bool g_Fullscreen                 = false;
D3D12_VIEWPORT                    g_viewPort;
D3D12_RECT                        g_scissorRect;
ComPtr<ID3D12Device>              g_device;
ComPtr<ID3D12Resource>            g_backBuffers[g_frameCount];
/*There must be at least one command allocator per render frame*/
ComPtr<ID3D12CommandAllocator>    g_commandAllocators[g_frameCount];
ComPtr<ID3D12CommandQueue>        g_commandQueue;
ComPtr<ID3D12RootSignature>       g_rootSignature;
/*A descriptor heap can be visualized as an array of descriptors (views).
A view simply describes a resource that resides in GPU memory.*/
ComPtr<ID3D12DescriptorHeap>      g_rtvHeap;
ComPtr<ID3D12DescriptorHeap>      g_dsvHeap;
ComPtr<ID3D12PipelineState>       g_pipelineState;
ComPtr<ID3D12GraphicsCommandList> g_commandList;
ComPtr<IDXGISwapChain4>           g_swapChain;
ComPtr<IDXGIFactory7>             g_factory;
/*In order to correctly offset the index into the descriptor heap,
the size of a single element in the descriptor heap needs to be queried during initialization*/
UINT                              g_rtvDescriptorSize;
/*Depending on the flip model of the swap chain,
the index of the current back buffer in the swap chain may not be sequential.*/
UINT                              g_CurrentBackBufferIndex;
//                                Synchronization objects
ComPtr<ID3D12Fence>               g_fence[g_frameCount];
UINT64                            g_fenceValue = NULL;
/*Used to keep track of the fence values that were used to signal the command queue for a particular frame.*/
UINT64                            g_frameFenceValues[g_frameCount] = {};
/*A handle to an OS event object that will be used to receive the notification that the fence has reached a specific value.*/
HANDLE                            g_fenceEvent;
// App resources.
ComPtr<ID3D12Resource>            g_vertexBuffer;
ComPtr<ID3D12Resource>            g_depthStencilBuffer;
D3D12_VERTEX_BUFFER_VIEW          g_vertexBufferView;
#pragma endregion
/*------------------*/ 

/*-------------------*/
/*Function Prototypes*/
/*-------------------*/
#pragma region Function Prototypes
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void EnableDebugLayer();
ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);
ComPtr<ID3D12Device2> MakeDevice(ComPtr<IDXGIAdapter4> adapter);
ComPtr<ID3D12CommandQueue> MakeCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
ComPtr<IDXGISwapChain4> MakeSwapChain(HWND hwnd, ComPtr<ID3D12CommandQueue> commandQueue, UINT32 width, UINT32 height, UINT32 bufferCount);
ComPtr <ID3D12DescriptorHeap> MakeDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT32 numDescriptors);
void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
bool CheckTearingSupport();
ComPtr<ID3D12CommandAllocator> MakeCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
ComPtr<ID3D12GraphicsCommandList> MakeCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);
ComPtr<ID3D12Fence> MakeFence(ComPtr<ID3D12Device2> device);
HANDLE MakeEventHandle();/*An OS event handle is used to block the CPU thread until the fence has been signaled*/
UINT64 Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, UINT64& fenceValue);
void WaitForFenceValue(ComPtr<ID3D12Fence> fence, UINT64 fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, UINT64& fenceValue, HANDLE fenceEvent);
#pragma endregion
/*-------------------*/

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    LoadString(hInstance, IDS_ENGINENAME, windowTitle, MAX_NAME_STRING);
    LoadString(hInstance, IDS_WINDOWCLASS, className, MAX_NAME_STRING);
    
    hIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_MAINICON));

    WNDCLASS wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hIcon = hIcon;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = className;
    RegisterClass(&wc);
    RECT windowRect = {};
    windowRect.right = CLIENT_WIDTH;
    windowRect.bottom = CLIENT_HEIGHT;

    int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
    
    /* Calculates the required size of the window rectangle, based on the desired client-rectangle size. */
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
    /* rc converted into a whole window coords from client coords */
    hwnd = CreateWindow((LPCWSTR)className, (LPCWSTR)windowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, NULL, NULL, NULL, hInstance,NULL);
    if (hwnd == NULL) return EXIT_FAILURE;

    int X = (screenWidth / 2) - ((windowRect.right - windowRect.left) / 2);
    int Y = (screenHeight / 2) - ((windowRect.bottom - windowRect.top) / 2);
    MoveWindow(hwnd, X, Y, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,FALSE);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    EnableDebugLayer();
    ComPtr<IDXGIAdapter4> adapter4 = GetAdapter(FALSE);
    ComPtr<ID3D12Device2> device2 = MakeDevice(adapter4);
    ComPtr<ID3D12CommandQueue> commandQueue = MakeCommandQueue(device2, D3D12_COMMAND_LIST_TYPE_DIRECT);
    ComPtr<IDXGISwapChain4> swapchain = MakeSwapChain(hwnd, commandQueue, NULL, NULL, g_frameCount);
    auto rtvHeap = MakeDescriptorHeap(device2, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, g_frameCount);
    UpdateRenderTargetViews(device2, swapchain, rtvHeap);
    ComPtr<ID3D12CommandAllocator> commandAllocator = MakeCommandAllocator(device2, D3D12_COMMAND_LIST_TYPE_DIRECT);

    CheckTearingSupport();
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	return EXIT_SUCCESS;
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // All painting occurs here, between BeginPaint and EndPaint.

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);
    }
    return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
void EnableDebugLayer()
{
    ComPtr<ID3D12Debug6> m_debugInterface;
#if defined(_DEBUG)
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&m_debugInterface)));
    m_debugInterface->EnableDebugLayer();
#endif
}

ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp)
{
    ComPtr<IDXGIAdapter1> adapter1;
    ComPtr<IDXGIAdapter4> adapter4;
    UINT createFactoryFlags = NULL;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&g_factory)));
    if (useWarp) {
        ThrowIfFailed(g_factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter1)));
        ThrowIfFailed(adapter1.As(&adapter4));
    }
    else {
        SIZE_T maxDedicatedVideoMemory = NULL;
        for (UINT i = NULL; g_factory->EnumAdapters1(i, &adapter1) != DXGI_ERROR_NOT_FOUND; i++) {
            /*Check to see if the adapter can create a D3D12 device without actually
             creating it. The adapter with the largest dedicated video memory
             is favored.*/ 
            DXGI_ADAPTER_DESC1 adapterDesc{};
            adapter1->GetDesc1(&adapterDesc);
            if ((adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == NULL &&
                SUCCEEDED(
                    D3D12CreateDevice(adapter1.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)
                ) && adapterDesc.DedicatedVideoMemory > maxDedicatedVideoMemory)
            {
                maxDedicatedVideoMemory = adapterDesc.DedicatedVideoMemory;
                ThrowIfFailed(adapter1.As(&adapter4));
            }
        }
    }
    return adapter4;
}

ComPtr<ID3D12Device2> MakeDevice(ComPtr<IDXGIAdapter4> adapter)
{
    ComPtr<ID3D12Device2> device2;
    ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device2)));
#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue1> pInfoQueue;
    if (SUCCEEDED(device2.As(&pInfoQueue))) {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };
        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
        };
        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;
        ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
    }
#endif
    return device2;
}

ComPtr<ID3D12CommandQueue> MakeCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandQueue> commandQueue;
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)));
    return commandQueue;
}

ComPtr<IDXGISwapChain4> MakeSwapChain(HWND hwnd, ComPtr<ID3D12CommandQueue> commandQueue, UINT32 width, UINT32 height, UINT32 bufferCount)
{
    ComPtr<IDXGISwapChain4> swapChain4;
    ComPtr<IDXGISwapChain1> swapChain1;
    ComPtr<IDXGIFactory7> factory;
    UINT createFactoryFlags = NULL;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&factory)));
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = bufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    // It is recommended to always allow tearing if tearing support is available.
    swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : NULL;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
    ThrowIfFailed(swapChain1.As(&swapChain4));
    //factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    return swapChain4;
}

ComPtr<ID3D12DescriptorHeap> MakeDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT32 numDescriptors)
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;
    ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));
    return descriptorHeap;
}

void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < g_frameCount; i++) {
        ComPtr<ID3D12Resource> backBuffer;
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
        g_backBuffers[i] = backBuffer;
        rtvHandle.Offset(rtvDescriptorSize);
    }
}

bool CheckTearingSupport()
{
    bool allowTearing = FALSE;
    ComPtr<IDXGIFactory7> factory;
    CreateDXGIFactory2(NULL,IID_PPV_ARGS(&factory));
    if (FAILED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))) {
        return allowTearing == FALSE;
    }
    else {
        return allowTearing == TRUE;
    }
}

ComPtr<ID3D12CommandAllocator> MakeCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));
    return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList> MakeCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12GraphicsCommandList> commandList;
    ThrowIfFailed(device->CreateCommandList(NULL, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
    /*Before the command list can be reset, it must first be closed. */
    ThrowIfFailed(commandList->Close());
    return commandList;
}

ComPtr<ID3D12Fence> MakeFence(ComPtr<ID3D12Device2> device)
{
    ComPtr<ID3D12Fence> fence;
    ThrowIfFailed(device->CreateFence(NULL, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    return fence;
}

HANDLE MakeEventHandle()
{
    HANDLE fenceEvent;
    fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(fenceEvent && "Failed to create fence event.");
    return fenceEvent;
}

UINT64 Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, UINT64& fenceValue)
{
    UINT64 fenceValueForSignal = fenceValue++;
    ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));
    return fenceValueForSignal;
}

void WaitForFenceValue(ComPtr<ID3D12Fence> fence, UINT64 fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max())
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
    }
}

void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, UINT64& fenceValue, HANDLE fenceEvent)
{
    UINT64 fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
    WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
}
