//            Window Settings.
UINT16  CLIENT_WIDTH = 640;
UINT16  CLIENT_HEIGHT = 480;
UINT16  currClientWidth = CLIENT_WIDTH;
UINT16  currClientHeight = CLIENT_HEIGHT;
float viewportWidth, viewportHeight;
HICON   hIcon;
WCHAR   windowTitle[MAX_NAME_STRING];
WCHAR   className[MAX_NAME_STRING];
HWND    hwnd;
BOOL    g_shouldClose;
ImGuiIO g_io;
/*Window rectangle(used to toggle fullscreen state).
 used to store the previous window dimensions before going to fullscreen mode.*/
RECT	g_WindowRect;

/* The number of swap chain back buffers*/
static const UINT16 g_frameCount = 2;
/* Use WARP adapter*/
bool g_useWarp = false;
/*Set to true once the DX12 objects have been initialized.*/
bool g_isInitialized = false;
bool g_VSync = true;
bool g_tearingSupported = false;
bool g_fullscreen = false;
float g_FoV = 45;
bool g_enableMsaa = false;
bool g_appPaused = false;
bool g_resizing = false;
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
ComPtr<ID3D12DescriptorHeap>      g_srvHeap;
ComPtr<ID3D12PipelineState>       g_pipelineState;
ComPtr<ID3D12GraphicsCommandList> g_commandList;
ComPtr<IDXGISwapChain4>           g_swapChain;
ComPtr<IDXGIFactory7>             g_factory;
/*In order to correctly offset the index into the descriptor heap,
the size of a single element in the descriptor heap needs to be queried during initialization*/
UINT                              g_rtvDescriptorSize;
/*Depending on the flip model of the swap chain,
the index of the current back buffer in the swap chain may not be sequential.*/
UINT                              g_currentBackBufferIndex;
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