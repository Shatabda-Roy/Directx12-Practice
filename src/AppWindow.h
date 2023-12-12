#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wrl.h>
#include <d3dx12.h>
#include <dxgi1_6.h>
#include <cstdint>
#include <EventSys.h>
class AppWindow
{
public:
	/*Number of Swapchain back buffers.*/
	static const uint16_t BufferCount = 3;
	/**
	* Get a handle to this window's instance.
	* @returns The handle to the window instance or nullptr if this is not a valid window.
	*/
	HWND GetWindowHandle() const;
	
	void Destroy();
	
	const WCHAR GetWindowName() const;
	
	uint16_t GetClientWidth() const;
	uint16_t GetClientHeight() const;

	/*Should this window be rendered with vertical refresh synchronization.*/
	bool IsVSync() const;
	void SetVSync(bool vSync);
	void ToggleVSync();

	/*Is this a windowed window or full-screen?*/
	bool IsFullScreen() const;

	// Set the fullscreen state of the window.
	void SetFullscreen(bool fullscreen);
	void ToggleFullscreen();

	/*Show this window.*/
	void Show();

	/*Hide the window.*/
	void Hide();

	/*Return the current back buffer index.*/
	UINT GetCurrentBackBufferIndex() const;

	/**
	* Present the swapchain's back buffer to the screen.
	* Returns the current back buffer index after the present.
	*/
	UINT Present();
	
	/*Get the render target view for the current back buffer.*/
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;

	/*Get the back buffer resource for the current back buffer.*/
	Microsoft::WRL::ComPtr<ID3D12Resource> GetCurrentBackBuffer() const;
protected:
	// The Window procedure needs to call protected methods of this class.
	friend LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	// Only the application can create a window.
	friend class Application;
	AppWindow() = delete;
	AppWindow(HWND hwnd, const WCHAR windowName, int clientWidth, int clientHeight, bool vSync);
	virtual ~AppWindow();

	// Update and Draw can only be called by the application.
	virtual void OnUpdate();
	virtual void OnRender();

	// The window was resized.
	virtual void OnResize();

	// Create the swapchian.
	Microsoft::WRL::ComPtr<IDXGISwapChain4> CreateSwapChain();

	// Update the render target views for the swapchain back buffers.
	void UpdateRenderTargetViews();
private:
	HRESULT CreateRenderWindow();
	// Windows should not be copied.
	AppWindow(const AppWindow& copy) = delete;
	AppWindow& operator=(const AppWindow& other) = delete;
	HWND m_hWnd;
	WCHAR m_WindowName;
	int m_ClientWidth;
	int m_ClientHeight;
	bool m_VSync;
	bool m_Fullscreen;
	uint64_t m_FrameCounter;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_dxgiSwapChain;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_d3d12RTVDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_d3d12BackBuffers[BufferCount];

	UINT m_RTVDescriptorSize;
	UINT m_CurrentBackBufferIndex;

	RECT m_WindowRect;
	bool m_IsTearingSupported;
};