#include "Application.h"
void Application::Initiate()
{
    struct VertexPosColor {
        XMFLOAT3 Positon;
        XMFLOAT3 Color;
    };

    static VertexPosColor g_Vertices[8] =
    {
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
    InitiateDebugLayer();
    /* Get GPU adapter with the heighest VRAM*/
    dxgiAdapter = GetAdapter(FALSE);
    /* Make a device on our selected GPU*/
    d3d12Device = MakeDevice(dxgiAdapter);
    /* Checking for Multisample support.*/
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS _msQualityLevels{};
    _msQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    _msQualityLevels.SampleCount = 4;
    _msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    DX::ThrowIfFailed(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &_msQualityLevels, sizeof(_msQualityLevels)));
    auto _m4xMsaaQuality = _msQualityLevels.NumQualityLevels;
    assert(_m4xMsaaQuality > NULL && "Unexpected MSAA quality level.");
    CommandQueue = MakeCommandQueue(d3d12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    SwapChain = MakeSwapChain(m_hWnd, CommandQueue, NULL, NULL, FrameCount);
    CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
    /* A descriptor heap is a collection of contiguous allocations of descriptors, one allocation for every descriptor.*/
    rtvHeap = MakeDescriptorHeap(d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, FrameCount, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    /* Gets the size of the handle increment for the given type of descriptor heap.*/
    rtvDescriptorSize = d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    /* Gain access to the back buffers.*/
    UpdateRenderTargetViews(d3d12Device, SwapChain, rtvHeap);
    /* Represents the allocations of storage for graphics processing unit (GPU) commands.*/
    for (int i = 0; i < FrameCount; ++i)
    {
        /* In order to achieve maximum frame-rates for the application,
        one command allocator per “in-flight” command list should be created.*/
        CommandAllocators[i] = MakeCommandAllocator(d3d12Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
    /*Encapsulates a list of graphics commands for rendering. Includes APIs for instrumenting the command list execution,
    and for setting and clearing the pipeline state.*/
    CommandList = MakeCommandList(d3d12Device, CommandAllocators[CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);
    isTearingSupported = CheckTearingSupport();
    g_fence = MakeFence(d3d12Device);
    g_fenceEvent = MakeEventHandle();

    DX::ThrowIfFailed(CommandList->Reset(CommandAllocators[CurrentBackBufferIndex].Get(), nullptr));

    /* Temporary vertex buffer resource that is used to transfer the CPU vertex buffer data to the GPU.*/
    ComPtr<ID3D12Resource> intermediateVertexBuffer;
    UpdateBufferResource(CommandList.Get(), &g_vertexBuffer, &intermediateVertexBuffer, _countof(g_Vertices), sizeof(VertexPosColor), g_Vertices);
    g_vertexBuffer->SetName(L"Vertex Buffer Resource Heap");
    // transition the vertex buffer data from copy destination state to vertex buffer state
    TransitionResource(CommandList.Get(), g_vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    // Create the vertex buffer view.
    g_vertexBufferView.BufferLocation = g_vertexBuffer->GetGPUVirtualAddress();
    g_vertexBufferView.SizeInBytes = sizeof(g_Vertices);
    g_vertexBufferView.StrideInBytes = sizeof(VertexPosColor);
    // Upload index buffer data.
    ComPtr<ID3D12Resource> intermediateIndexBuffer;
    UpdateBufferResource(CommandList.Get(), &g_indexBuffer, &intermediateIndexBuffer, _countof(g_Indicies), sizeof(WORD), g_Indicies);
    g_indexBuffer->SetName(L"Index Buffer Resource Heap");
    // transition the vertex buffer data from copy destination state to vertex buffer state
    TransitionResource(CommandList.Get(), g_indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
    // Create index buffer view.
    g_indexBufferView.BufferLocation = g_indexBuffer->GetGPUVirtualAddress();
    g_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
    g_indexBufferView.SizeInBytes = sizeof(g_Indicies);

    // Create the descriptor heap for the depth-stencil view.
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DX::ThrowIfFailed(d3d12Device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap)));
    // Load the vertex shader.
    ComPtr<ID3DBlob> vertexShaderBlob;
    DX::ThrowIfFailed(D3DReadFileToBlob(L"A:/Work/DEVELOPMENT/Directx12-Practice/bin/Directx12-Practice/Debug/vert.cso", &vertexShaderBlob));

    // Load the pixel shader.
    ComPtr<ID3DBlob> pixelShaderBlob;
    DX::ThrowIfFailed(D3DReadFileToBlob(L"A:/Work/DEVELOPMENT/Directx12-Practice/bin/Directx12-Practice/Debug/frag.cso", &pixelShaderBlob));
    // Create the vertex input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    // Create a root signature.
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    if (FAILED(d3d12Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
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
    DX::ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
        featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
    // Create the root signature.
    DX::ThrowIfFailed(d3d12Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
        rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&RootSignature)));
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

    pipelineStateStream.pRootSignature = RootSignature.Get();
    pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
    pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
    pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
    pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelineStateStream.RTVFormats = rtvFormats;

    D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = { sizeof(PipelineStateStream), &pipelineStateStream };
    DX::ThrowIfFailed(d3d12Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&PipelineState)));
    DX::ThrowIfFailed(CommandList->Close());
    ID3D12CommandList* const commandLists[] = {
        CommandList.Get()
    };
    CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
    Flush(CommandQueue, g_fence, g_fenceValue, g_fenceEvent);
    srvHeap = MakeDescriptorHeap(d3d12Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    g_isInitialized = true;
    /* Needs the directx device intialized to function.*/
    ResizeDepthBuffer(640, 480);
}

void Application::Update()
{
}

void Application::Destroy()
{
}
void Application::InitiateDebugLayer()
{
    ComPtr<ID3D12Debug6> _debugInterface;
#if defined(_DEBUG)
    DX::ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&_debugInterface)));
    _debugInterface->EnableDebugLayer();
#endif
}

ComPtr<IDXGIAdapter4> Application::GetAdapter(bool useWarp)
{
    ComPtr<IDXGIAdapter1> _adapter1;
    ComPtr<IDXGIAdapter4> _adapter4;
    UINT _createFactoryFlags = NULL;
#if defined(_DEBUG)
    _createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    DX::ThrowIfFailed(CreateDXGIFactory2(_createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
    if (useWarp) {
        DX::ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&_adapter1)));
        DX::ThrowIfFailed(_adapter1.As(&_adapter4));
    }
    else {
        SIZE_T _maxDedicatedVideoMemory = NULL;
        for (UINT i = NULL; dxgiFactory->EnumAdapters1(i, &_adapter1) != DXGI_ERROR_NOT_FOUND; i++) {
            /*Check to see if the adapter can create a D3D12 device without actually
             creating it. The adapter with the largest dedicated video memory
             is favored.*/
            DXGI_ADAPTER_DESC1 _adapterDesc{};
            _adapter1->GetDesc1(&_adapterDesc);
            if ((_adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == NULL &&
                SUCCEEDED(
                    D3D12CreateDevice(_adapter1.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)
                ) && _adapterDesc.DedicatedVideoMemory > _maxDedicatedVideoMemory)
            {
                _maxDedicatedVideoMemory = _adapterDesc.DedicatedVideoMemory;
                DX::ThrowIfFailed(_adapter1.As(&_adapter4));
            }
        }
    }
    return _adapter4;
}
ComPtr<ID3D12Device2> Application::MakeDevice(ComPtr<IDXGIAdapter4> adapter)
{
    ComPtr<ID3D12Device2> _device2;
    DX::ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&_device2)));
#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue1> _pInfoQueue;
    if (SUCCEEDED(_device2.As(&_pInfoQueue))) {
        _pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        _pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        //pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY _Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };
        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID _DenyIds[] = {
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
        };
        D3D12_INFO_QUEUE_FILTER _NewFilter = {};
        _NewFilter.DenyList.NumSeverities = _countof(_Severities);
        _NewFilter.DenyList.pSeverityList = _Severities;
        _NewFilter.DenyList.NumIDs = _countof(_DenyIds);
        _NewFilter.DenyList.pIDList = _DenyIds;
        DX::ThrowIfFailed(_pInfoQueue->PushStorageFilter(&_NewFilter));
    }
#endif
    return _device2;
}
ComPtr<IDXGISwapChain4> Application::MakeSwapChain(HWND hwnd, ComPtr<ID3D12CommandQueue> commandQueue, UINT32 width, UINT32 height, UINT32 bufferCount)
{
    ComPtr<IDXGISwapChain4> _swapChain4;
    ComPtr<IDXGISwapChain1> _swapChain1;
    ComPtr<IDXGIFactory7> _factory;
    UINT _createFactoryFlags = NULL;
#if defined(_DEBUG)
    _createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    DX::ThrowIfFailed(CreateDXGIFactory2(_createFactoryFlags, IID_PPV_ARGS(&_factory)));
    DXGI_SWAP_CHAIN_DESC1 _swapChainDesc{};
    _swapChainDesc.Width = width;
    _swapChainDesc.Height = height;
    _swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    _swapChainDesc.Stereo = FALSE;
    _swapChainDesc.SampleDesc.Count = 1;
    _swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    _swapChainDesc.BufferCount = bufferCount;
    _swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    _swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    _swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    // It is recommended to always allow tearing if tearing support is available.
    _swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : NULL;
    DX::ThrowIfFailed(_factory->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &_swapChainDesc, nullptr, nullptr, &_swapChain1));
    DX::ThrowIfFailed(_swapChain1.As(&_swapChain4));
    _factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    return _swapChain4;
}
bool Application::CheckTearingSupport()
{
    bool allowTearing = FALSE;
    ComPtr<IDXGIFactory7> factory;
    CreateDXGIFactory2(NULL, IID_PPV_ARGS(&factory));
    if (FAILED(factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))) {
        return allowTearing == FALSE;
    }
    else {
        return allowTearing == TRUE;
    }
}

ComPtr<ID3D12DescriptorHeap> Application::MakeDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT32 numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;
    desc.Flags = flags;
    DX::ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)));
    return descriptorHeap;
}
ComPtr<ID3D12CommandQueue> Application::MakeCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandQueue> commandQueue;
    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    DX::ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&commandQueue)));
    return commandQueue;
}
/* Create an RTV to each buffer in the swap chain. */
void Application::UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    /* Gets the CPU descriptor handle that represents the start of the heap.*/
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < FrameCount; i++) {
        ComPtr<ID3D12Resource> backBuffer;
        /* Accesses one of the swap-chain's back buffers */
        DX::ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
        /* Creates a render-target view for accessing resource data.*/
        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
        BackBuffers[i] = backBuffer;
        rtvHandle.Offset(rtvDescriptorSize);
    }
}
ComPtr<ID3D12CommandAllocator> Application::MakeCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    DX::ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));
    return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList> Application::MakeCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12GraphicsCommandList> commandList;
    DX::ThrowIfFailed(device->CreateCommandList(NULL, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
    /* Before the command list can be reset, it must first be closed. */
    DX::ThrowIfFailed(commandList->Close());
    return commandList;
}

ComPtr<ID3D12Fence> Application::MakeFence(ComPtr<ID3D12Device2> device)
{
    ComPtr<ID3D12Fence> fence;
    DX::ThrowIfFailed(device->CreateFence(NULL, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    return fence;
}

HANDLE Application::MakeEventHandle()
{
    HANDLE fenceEvent;
    fenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(fenceEvent && "Failed to create fence event.");
    return fenceEvent;
}
void Application::UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList> commandList, ID3D12Resource** pDestinationResource,
    ID3D12Resource** pIntermediateResource, size_t numElements, size_t elementSize, const void* bufferData, D3D12_RESOURCE_FLAGS flags)
{
    size_t bufferSize = numElements * elementSize;
    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto buffer = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
    // Create a committed resource for the GPU resource in a default heap.
    DX::ThrowIfFailed(d3d12Device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &buffer,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(pDestinationResource)));
    // Create an committed resource for the upload.
    if (bufferData != NULL)
    {
        heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        buffer = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
        DX::ThrowIfFailed(d3d12Device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE,
            &buffer, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(pIntermediateResource)));

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = bufferData;
        subresourceData.RowPitch = bufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        UpdateSubresources(commandList.Get(), *pDestinationResource, *pIntermediateResource, 0, 0, 1, &subresourceData);
    }
}
void Application::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList, Microsoft::WRL::ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.Get(), beforeState, afterState);
    commandList->ResourceBarrier(1, &barrier);
}
void Application::Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, UINT64& fenceValue, HANDLE fenceEvent)
{
    UINT64 fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
    WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
}
UINT64 Application::Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, UINT64& fenceValue)
{
    UINT64 fenceValueForSignal = fenceValue++;
    DX::ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));
    return fenceValueForSignal;
}
void Application::WaitForFenceValue(ComPtr<ID3D12Fence> fence, UINT64 fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration)
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        DX::ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
        ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
    }
}
void Application::ResizeDepthBuffer(int width, int height)
{
    if (g_isInitialized) {
        /* Ensure that there are no command lists that could be referencing the current depth buffer are “in-flight” on the command queue.*/
        Flush(CommandQueue, g_fence, g_fenceValue, g_fenceEvent);
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
        DX::ThrowIfFailed(d3d12Device->CreateCommittedResource(
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

        d3d12Device->CreateDepthStencilView(g_depthBuffer.Get(), &dsv, dsvHeap->GetCPUDescriptorHandleForHeapStart());
    }
}