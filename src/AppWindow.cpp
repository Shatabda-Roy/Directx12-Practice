#include "AppWindow.h"

AppWindow::AppWindow(HWND hwnd, const WCHAR windowName, int clientWidth, int clientHeight, bool vSync) 
	: m_hWnd{hwnd},
	m_WindowName{windowName},
	m_ClientWidth{clientWidth},
	m_ClientHeight{clientHeight},
	m_VSync{vSync},
	m_Fullscreen{false},
	m_FrameCounter{NULL}
{
	//m_dxgiSwapChain = CreateSwapChain();
	CreateRenderWindow();
}

AppWindow::~AppWindow()
{
}
void AppWindow::OnUpdate()
{
}
void AppWindow::OnRender()
{
}
void AppWindow::OnResize()
{
}
void AppWindow::Destroy()
{
}

HWND AppWindow::GetWindowHandle() const
{
	return m_hWnd;
}

const WCHAR AppWindow::GetWindowName() const
{
	return m_WindowName;
}

uint16_t AppWindow::GetClientWidth() const
{
	return m_ClientWidth;
}

uint16_t AppWindow::GetClientHeight() const
{
	return m_ClientHeight;
}

bool AppWindow::IsVSync() const
{
	return m_VSync;
}

void AppWindow::SetVSync(bool vSync)
{
	m_VSync = vSync;
}

void AppWindow::ToggleVSync()
{
	SetVSync(!m_VSync);
}

bool AppWindow::IsFullScreen() const
{
	return m_Fullscreen;
}

void AppWindow::SetFullscreen(bool fullscreen)
{

}

void AppWindow::ToggleFullscreen()
{
}

void AppWindow::Show()
{
	::ShowWindow(m_hWnd, SW_SHOW);
}

void AppWindow::Hide()
{
	::ShowWindow(m_hWnd, SW_HIDE);
}

UINT AppWindow::GetCurrentBackBufferIndex() const
{
	return 0;
}

UINT AppWindow::Present()
{
	return 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE AppWindow::GetCurrentRenderTargetView() const
{
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}

Microsoft::WRL::ComPtr<ID3D12Resource> AppWindow::GetCurrentBackBuffer() const
{
	return Microsoft::WRL::ComPtr<ID3D12Resource>();
}

Microsoft::WRL::ComPtr<IDXGISwapChain4> AppWindow::CreateSwapChain()
{
	return Microsoft::WRL::ComPtr<IDXGISwapChain4>();
}

void AppWindow::UpdateRenderTargetViews()
{
}

HRESULT AppWindow::CreateRenderWindow()
{
	HRESULT result = S_OK;
	
	return result;
}
