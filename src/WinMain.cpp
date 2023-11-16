/* WRITTEN BY SHATABDA ROY. */
#include "pch.hpp"
#include "AppTime.h"
#define MAX_NAME_STRING 256
using namespace DirectX;
using namespace Microsoft::WRL;

/*------------------*/
/* Global Variables */
/*------------------*/
#pragma region Global Variables
//            Window Settings.
UINT16 CLIENT_WIDTH     = 640;
UINT16 CLIENT_HEIGHT    = 480;
UINT16 currClientWidth  = CLIENT_WIDTH;
UINT16 currClientHeight = CLIENT_HEIGHT;
HICON         hIcon;
WCHAR         windowTitle[MAX_NAME_STRING];
WCHAR         className[MAX_NAME_STRING];
HWND          hwnd;
BOOL          g_shouldClose;
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
float g_FoV                       = 45;
bool g_enableMsaa                 = false;
bool g_appPaused                  = false;
bool g_resizing                   = false;
UINT                              g_m4xMsaaQuality;
Apparatus::Time                   g_time;
/*The viewport and scissor rect variables are used to initialize
the rasterizer stage of the rendering pipeline.*/
D3D12_VIEWPORT                    g_viewPort;
D3D12_RECT                        g_scissorRect;
ComPtr<ID3D12Device2>             g_device;
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
ComPtr<ID3D12Fence>               g_fence;
UINT64                            g_fenceValue = NULL;
/*Used to keep track of the fence values that were used to signal the command queue for a particular frame.*/
UINT64                            g_frameFenceValues[g_frameCount] = {};
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
void Update();
void Render();
void Resize(UINT32 width, UINT32 height);
void SetFullscreen(bool fullscreen);
void UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList> commandList, ID3D12Resource** pDestinationResource,
    ID3D12Resource** pIntermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
void ResizeDepthBuffer(int width, int height);
void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> resource,
    D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);
void CalculateFrameStats();
struct VertexPosColor {
    XMFLOAT3 Positon;
    XMFLOAT3 Color;
};
static VertexPosColor g_Vertices[8] = {
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
    { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
    { XMFLOAT3(1.0f,  1.0f, -1.0f),  XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
    { XMFLOAT3(1.0f, -1.0f, -1.0f),  XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
    { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
    { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
    { XMFLOAT3(1.0f,  1.0f,  1.0f),  XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
    { XMFLOAT3(1.0f, -1.0f,  1.0f),  XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};
static UINT16 g_Indicies[36] =
{
    0, 1, 2, 0, 2, 3,
    4, 6, 5, 4, 7, 6,
    4, 5, 1, 4, 1, 0,
    3, 2, 6, 3, 6, 7,
    1, 5, 6, 1, 6, 2,
    4, 0, 3, 4, 3, 7
};
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

    EnableDebugLayer();
    g_viewPort = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(CLIENT_WIDTH), static_cast<float>(CLIENT_HEIGHT));
    g_scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
    /* Get GPU adapter with the heighest VRAM*/
    ComPtr<IDXGIAdapter4> adapter4 = GetAdapter(FALSE);
    /* Make a device on our selected GPU*/
    g_device = MakeDevice(adapter4);
    /* Checking for Multisample support.*/
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels{};
    msQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    msQualityLevels.SampleCount = 4;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    ThrowIfFailed(g_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,&msQualityLevels,sizeof(msQualityLevels)));
    g_m4xMsaaQuality = msQualityLevels.NumQualityLevels;
    assert(g_m4xMsaaQuality > NULL && "Unexpected MSAA quality level.");
    /* Provides methods for submitting command lists, synchronizing command list execution,
    instrumenting the command queue, and updating resource tile mappings.*/
    g_commandQueue = MakeCommandQueue(g_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    /* Swap chains control the back buffer rotation.*/
    g_swapChain = MakeSwapChain(hwnd, g_commandQueue, NULL, NULL, g_frameCount);
    /* Returns the index of the current back buffer to the app.*/
    g_CurrentBackBufferIndex = g_swapChain->GetCurrentBackBufferIndex();
    /* A descriptor heap is a collection of contiguous allocations of descriptors, one allocation for every descriptor.*/
    g_rtvHeap = MakeDescriptorHeap(g_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, g_frameCount);
    /* Gets the size of the handle increment for the given type of descriptor heap.*/
    g_rtvDescriptorSize = g_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    /* Gain access to the back buffers.*/
    UpdateRenderTargetViews(g_device, g_swapChain, g_rtvHeap);
    /* Represents the allocations of storage for graphics processing unit (GPU) commands.*/
    for (int i = 0; i < g_frameCount; ++i)
    {
        /* In order to achieve maximum frame-rates for the application,
        one command allocator per “in-flight” command list should be created.*/
        g_commandAllocators[i] = MakeCommandAllocator(g_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
    /*Encapsulates a list of graphics commands for rendering. Includes APIs for instrumenting the command list execution,
    and for setting and clearing the pipeline state.*/
    g_commandList = MakeCommandList(g_device, g_commandAllocators[g_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);
    g_TearingSupported = CheckTearingSupport();
    g_fence = MakeFence(g_device);
    g_fenceEvent = MakeEventHandle();

    ThrowIfFailed(g_commandList->Reset(g_commandAllocators[g_CurrentBackBufferIndex].Get(), nullptr));

    /* Temporary vertex buffer resource that is used to transfer the CPU vertex buffer data to the GPU.*/
    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    UpdateBufferResource(g_commandList.Get(), &g_vertexBuffer, &intermediateVertexBuffer,_countof(g_Vertices), sizeof(VertexPosColor), g_Vertices);

    // Create the vertex buffer view.
    g_vertexBufferView.BufferLocation = g_vertexBuffer->GetGPUVirtualAddress();
    g_vertexBufferView.SizeInBytes = sizeof(g_Vertices);
    g_vertexBufferView.StrideInBytes = sizeof(VertexPosColor);
    // Upload index buffer data.
    ComPtr<ID3D12Resource> intermediateIndexBuffer;
    UpdateBufferResource(g_commandList.Get(), &g_indexBuffer, &intermediateIndexBuffer, _countof(g_Indicies), sizeof(WORD), g_Indicies);
    // Create index buffer view.
    g_indexBufferView.BufferLocation = g_indexBuffer->GetGPUVirtualAddress();
    g_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    g_indexBufferView.SizeInBytes = sizeof(g_Indicies);
    
    // Create the descriptor heap for the depth-stencil view.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(g_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&g_dsvHeap)));
    // Load the vertex shader.
    ComPtr<ID3DBlob> vertexShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"A:/Work/DEVELOPMENT/Directx12-Practice/bin/Directx12-Practice/Debug/vert.cso", &vertexShaderBlob));

    // Load the pixel shader.
    ComPtr<ID3DBlob> pixelShaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(L"A:/Work/DEVELOPMENT/Directx12-Practice/bin/Directx12-Practice/Debug/frag.cso", &pixelShaderBlob));
    // Create the vertex input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    // Create a root signature.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(g_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }
    // Allow input layout and deny unnecessary access to certain pipeline stages.
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
    // A single 32-bit constant root parameter that is used by the vertex shader.
    CD3DX12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].InitAsConstants(sizeof(XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
    rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);
    // Serialize the root signature.
    ComPtr<ID3DBlob> rootSignatureBlob;
    ComPtr<ID3DBlob> errorBlob;
    ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
        featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
    // Create the root signature.
    ThrowIfFailed(g_device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
        rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&g_rootSignature)));
    struct PipelineStateStream
    {
        CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
        CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
        CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
        CD3DX12_PIPELINE_STATE_STREAM_VS VS;
        CD3DX12_PIPELINE_STATE_STREAM_PS PS;
        CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
        CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
    } pipelineStateStream;

    D3D12_RT_FORMAT_ARRAY rtvFormats = {};
    rtvFormats.NumRenderTargets = 1;
    rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    pipelineStateStream.pRootSignature = g_rootSignature.Get();
    pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &pipelineStateStream };
    ThrowIfFailed(g_device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&g_pipelineState)));
    ThrowIfFailed(g_commandList->Close());
    ID3D12CommandList* const commandLists[] = {
    g_commandList.Get()
    };
    g_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
    Flush(g_commandQueue, g_fence, g_fenceValue, g_fenceEvent);

    g_IsInitialized = TRUE;
    ResizeDepthBuffer(CLIENT_WIDTH, CLIENT_HEIGHT);
    ShowWindow(hwnd, nCmdShow);
    g_time.Reset();
    MSG msg = { };
    while (!g_shouldClose)
    {
        g_time.Tick();
        if (PeekMessage(&msg, hwnd, NULL, NULL, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        CalculateFrameStats();

        if (g_appPaused) {
            Sleep(100);
        }
    }
    // Make sure the command queue has finished all commands before closing.
    Flush(g_commandQueue, g_fence, g_fenceValue, g_fenceEvent);

    ::CloseHandle(g_fenceEvent);
	return EXIT_SUCCESS;
}
void CalculateFrameStats() {
    // Code computes the average frames per second, and also the
    // average time it takes to render one frame. These stats
    // are appended to the window caption bar.
    static int frameCount{};
    static float timeElapsed{};
    frameCount++;
    if ((g_time.TotalTime() - timeElapsed) >= 1) {
        float fps = (float)frameCount;
        float mspf = 1000 / fps;
        std::wstring mspfStr = std::to_wstring(mspf);
        std::wstring fpsStr = std::to_wstring(frameCount);
        std::wstring windowText = (std::wstring)windowTitle + L" " + L"FPS : " + fpsStr + L" " + L"MSPF: " + mspfStr;
        SetWindowText(hwnd, windowText.c_str());
        auto nig = GetLastError();
        //reset for average
        frameCount = NULL;
        timeElapsed += 1;
    }
}
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
if (g_IsInitialized)
{
    switch (uMsg)
    {
    case WM_PAINT:
        Update();
        Render();
        break;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

        switch (wParam)
        {
        case 'V':
            g_VSync = !g_VSync;
            break;
        case VK_ESCAPE:
            ::PostQuitMessage(0);
            break;
        }
    }
    break;
    case WM_SIZE:
    {
        RECT clientRect = {};
        ::GetClientRect(hwnd, &clientRect);

        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;
        currClientWidth = width;
        currClientHeight = height;
        Resize(width, height);
    }
    break;
    case WM_ENTERSIZEMOVE:
        g_appPaused = true;
        g_resizing = true;
        g_time.Stop();
        return 0;
    case WM_EXITSIZEMOVE:
        g_appPaused = false;
        g_resizing = false;
        g_time.Start();
        return 0;
    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            g_appPaused = true;
            g_time.Stop();
        }
        else {
            g_appPaused = false;
            g_time.Start();
        }
        return 0;
    case WM_CLOSE:
        ::PostQuitMessage(0);
        g_shouldClose = TRUE;
        break;
    default:
        return ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}
else
{ return ::DefWindowProcW(hwnd, uMsg, wParam, lParam); }
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
        //pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

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
    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
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
/* Create an RTV to each buffer in the swap chain. */
void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    /* Gets the CPU descriptor handle that represents the start of the heap.*/
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < g_frameCount; i++) {
        ComPtr<ID3D12Resource> backBuffer;
        /* Accesses one of the swap-chain's back buffers */
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
        /* Creates a render-target view for accessing resource data.*/
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
    /* Before the command list can be reset, it must first be closed. */
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
void WaitForFenceValue(ComPtr<ID3D12Fence> fence, UINT64 fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration)
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
DirectX::XMMATRIX g_ProjectionMatrix;
DirectX::XMMATRIX g_ViewMatrix;
DirectX::XMMATRIX g_ModelMatrix;
void Update()
{
    static UINT64 frameCounter = NULL;
    static double elapsedSeconds = NULL;
    static std::chrono::high_resolution_clock clock;
    static auto t0 = clock.now();

    frameCounter++;
    auto t1 = clock.now();
    auto deltaTime = t1 - t0;
    t0 = t1;
    elapsedSeconds += deltaTime.count() * 1e-9;
    if (elapsedSeconds > 1.0)
    {
        char buffer[500];
        auto fps = frameCounter / elapsedSeconds;
        //sprintf_s(buffer, 500, "FPS: %f\n", fps);
        //OutputDebugString((LPCWSTR)buffer);

        frameCounter = NULL;
        elapsedSeconds = NULL;
    }
    // Update the model matrix.
    float angle = static_cast<float>(elapsedSeconds * 90.0);
    const XMVECTOR rotationAxis = XMVectorSet(0, 1, 1, 0);
    g_ModelMatrix = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(angle));

    // Update the view matrix.
    const XMVECTOR eyePosition = XMVectorSet(0, 0, -10, 1);
    const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
    const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);
    g_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

    // Update the projection matrix.
    float aspectRatio = currClientWidth / static_cast<float>(currClientHeight);
    g_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(g_FoV), aspectRatio, 0.1f, 100.0f);
}

void Render()
{
    auto commandAllocator = g_commandAllocators[g_CurrentBackBufferIndex];
    auto backBuffer = g_backBuffers[g_CurrentBackBufferIndex];

    commandAllocator->Reset();
    g_commandList->Reset(commandAllocator.Get(), nullptr);
    // Clear the render target.
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        g_commandList->ResourceBarrier(1, &barrier);
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(g_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            g_CurrentBackBufferIndex, g_rtvDescriptorSize);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(g_dsvHeap->GetCPUDescriptorHandleForHeapStart());

        g_commandList->ClearRenderTargetView(rtv, clearColor, NULL, nullptr);
        g_commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1, NULL, NULL, nullptr);
        /*The next step is to prepare the rendering pipeline for rendering.*/
        g_commandList->SetPipelineState(g_pipelineState.Get());
        /*Not explicitly setting the root signature on the command list
        will result in a runtime error while trying to bind resources.*/
        g_commandList->SetGraphicsRootSignature(g_rootSignature.Get());
        g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_commandList->IASetVertexBuffers(0, 1, &g_vertexBufferView);
        g_commandList->IASetIndexBuffer(&g_indexBufferView);
        g_commandList->RSSetViewports(1, &g_viewPort);
        /*the scissor rectangle must be explicitly specified */
        g_commandList->RSSetScissorRects(1, &g_scissorRect);
        g_commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
        // Update the MVP matrix
        XMMATRIX mvpMatrix = XMMatrixMultiply(g_ModelMatrix, g_ViewMatrix);
        mvpMatrix = XMMatrixMultiply(mvpMatrix, g_ProjectionMatrix);
        g_commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);
        g_commandList->DrawIndexedInstanced(_countof(g_Indicies), 1, 0, 0, 0);
    }
    // Present
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        g_commandList->ResourceBarrier(1, &barrier);
        ThrowIfFailed(g_commandList->Close());

        ID3D12CommandList* const commandLists[] = {
            g_commandList.Get()
        };
        g_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
        UINT syncInterval = g_VSync ? 1 : NULL;
        UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : NULL;
        ThrowIfFailed(g_swapChain->Present(syncInterval, presentFlags));

        g_frameFenceValues[g_CurrentBackBufferIndex] = Signal(g_commandQueue, g_fence, g_fenceValue);
        g_CurrentBackBufferIndex = g_swapChain->GetCurrentBackBufferIndex();

        WaitForFenceValue(g_fence, g_frameFenceValues[g_CurrentBackBufferIndex], g_fenceEvent);
    }
    /*
    Clear the color and depth buffers
    For each object in the scene:
        Transition any resources to the required state
        Bind the Pipeline State Object (PSO)
        Bind the Root Signature
        Bind any resources (CBV, UAV, SRV, Samplers, etc..) to the shader stages
        Set the primitive topology for the Input Assembler (IA)
        Bind the vertex buffer to the IA
        Bind the index buffer to the IA
        Set the viewport(s) for the Rasterizer Stage (RS)
        Set the scissor rectangle(s) for the RS
        Bind the color and depth-stencil render targets to the Output Merger (OM)
        Draw the geometry
    Present the rendered image to the screen
*/
}

void Resize(UINT32 width, UINT32 height)
{
    if (CLIENT_WIDTH != width || CLIENT_HEIGHT != height)
    {
        // Don't allow 0 size swap chain back buffers.
        CLIENT_WIDTH = std::max(1u, width);
        CLIENT_HEIGHT = std::max(1u, height);

        // Flush the GPU queue to make sure the swap chain's back buffers
        // are not being referenced by an in-flight command list.
        Flush(g_commandQueue, g_fence, g_fenceValue, g_fenceEvent);
        for (int i = 0; i < g_frameCount; ++i)
        {
            // Any references to the back buffers must be released
            // before the swap chain can be resized.
            g_backBuffers[i].Reset();
            g_frameFenceValues[i] = g_frameFenceValues[g_CurrentBackBufferIndex];
        }
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        ThrowIfFailed(g_swapChain->GetDesc(&swapChainDesc));
        ThrowIfFailed(g_swapChain->ResizeBuffers(g_frameCount, CLIENT_WIDTH, CLIENT_HEIGHT,
            swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

        g_CurrentBackBufferIndex = g_swapChain->GetCurrentBackBufferIndex();

        UpdateRenderTargetViews(g_device.Get(), g_swapChain, g_rtvHeap);
        g_viewPort = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
        ResizeDepthBuffer(width, height);
    }
}

void SetFullscreen(bool fullscreen)
{
    if (g_Fullscreen != fullscreen)
    {
        g_Fullscreen = fullscreen;

        if (g_Fullscreen) // Switching to fullscreen.
        {
            // Store the current window dimensions so they can be restored 
            // when switching out of fullscreen state.
            ::GetWindowRect(hwnd, &g_WindowRect);
            // Set the window style to a borderless window so the client area fills
// the entire screen.
            UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

            ::SetWindowLongW(hwnd, GWL_STYLE, windowStyle);
            // Query the name of the nearest display device for the window.
// This is required to set the fullscreen dimensions of the window
// when using a multi-monitor setup.
            HMONITOR hMonitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize = sizeof(MONITORINFOEX);
            ::GetMonitorInfo(hMonitor, &monitorInfo);
            ::SetWindowPos(hwnd, HWND_TOP,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(hwnd, SW_MAXIMIZE);
        }
        else
        {
            // Restore all the window decorators.
            ::SetWindowLong(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

            ::SetWindowPos(hwnd, HWND_NOTOPMOST,
                g_WindowRect.left,
                g_WindowRect.top,
                g_WindowRect.right - g_WindowRect.left,
                g_WindowRect.bottom - g_WindowRect.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(hwnd, SW_NORMAL);
        }
    }
}

void UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList> commandList, ID3D12Resource** pDestinationResource,
    ID3D12Resource** pIntermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
    size_t bufferSize = numElements * elementSize;
    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto buffer = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
    // Create a committed resource for the GPU resource in a default heap.
    ThrowIfFailed(g_device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &buffer,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(pDestinationResource)));
    // Create an committed resource for the upload.
    if (bufferData != NULL)
    {
        heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        buffer = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        ThrowIfFailed(g_device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
            &buffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(pIntermediateResource)));
        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = bufferData;
        subresourceData.RowPitch = bufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;
        UpdateSubresources(commandList.Get(), *pDestinationResource, *pIntermediateResource, 0, 0, 1, &subresourceData);
    }
}

void ResizeDepthBuffer(int width, int height)
{
    if (g_IsInitialized) {
        /* Ensure that there are no command lists that could be referencing the current depth buffer are “in-flight” on the command queue.*/
        Flush(g_commandQueue, g_fence, g_fenceValue, g_fenceEvent);
        /*If the window is minimized, it is possible that either the width or the height of the client area of the window becomes 0. 
        Creating a resource with a 0 size is an error so to prevent the buffer from being created with a 0 size,
        the width and height are clamped to a size of at least 1 in both dimensions.*/
        width = std::max(1, width);
        height = std::max(1, height);
        // Resize screen dependent resources.
        // Create a depth buffer.
        D3D12_CLEAR_VALUE optimizedClearValue = {};
        optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
        optimizedClearValue.DepthStencil = { 1.0f, 0 };
        auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        auto texBuffer = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
            1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
        ThrowIfFailed(g_device->CreateCommittedResource(
            &heapProp,
            D3D12_HEAP_FLAG_NONE,
            &texBuffer,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &optimizedClearValue,
            IID_PPV_ARGS(&g_depthBuffer)
        ));
        // Update the depth-stencil view.
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
        dsv.Format = DXGI_FORMAT_D32_FLOAT;
        dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsv.Texture2D.MipSlice = NULL;
        dsv.Flags = D3D12_DSV_FLAG_NONE;

        g_device->CreateDepthStencilView(g_depthBuffer.Get(), &dsv, g_dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }
}

void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(),beforeState, afterState);
    commandList->ResourceBarrier(1, &barrier);
}
