#include <Windows.h>
#include <assert.h>
#include "Application.h"
#include "../resource.h"
#define MAX_NAME_STRING 256
#define HInstance() GetModuleHandle(NULL)
WCHAR WindowClass[MAX_NAME_STRING];
WCHAR WindowTitle[MAX_NAME_STRING];
INT   WindowClientWidth;
INT   WindowClientHeight;
BOOL  ShouldClose = FALSE;
HWND  hWnd;

#pragma region Function Prototypes
auto InitiateWindow() -> HWND;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#pragma endregion

_Use_decl_annotations_ int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, INT)
{
	hWnd = InitiateWindow();
	Application::GetInstance().SetHWND(hWnd);
	Application::GetInstance().Initiate();
	MSG _msg{};
	while (!ShouldClose) {
		if (PeekMessage(&_msg, hWnd, NULL, NULL, PM_REMOVE)) {
			TranslateMessage(&_msg);
			DispatchMessage(&_msg);
		}
		Application::GetInstance().Update();
		Sleep(100);
	}
	Application::GetInstance().Destroy();
	return EXIT_SUCCESS;
}
HWND InitiateWindow()
{
	HICON hIcon = LoadIcon(HInstance(),MAKEINTRESOURCE(IDI_XICON));
	LoadString(HInstance(), IDS_WINDOWCLASS, WindowClass, MAX_NAME_STRING);
	LoadString(HInstance(), IDS_WINDOWTITLE, WindowTitle, MAX_NAME_STRING);

	WindowClientWidth  = 640;
	WindowClientHeight = 480;

	/* Create Window Class.*/
	WNDCLASSEXW _wc{};
	_wc.cbSize = sizeof(WNDCLASSEXW);
	_wc.style = CS_HREDRAW | CS_VREDRAW;
	_wc.lpfnWndProc = WindowProc;
	_wc.cbWndExtra = DLGWINDOWEXTRA;
	_wc.hInstance = HInstance();
	_wc.lpszMenuName = nullptr;
	_wc.hIcon = hIcon;
	_wc.hIconSm = hIcon;
	_wc.lpszClassName = WindowClass;
	RegisterClassEx(&_wc);

	RECT _windowRect = {};
	_windowRect.right = WindowClientWidth;
	_windowRect.bottom = WindowClientHeight;

	int _screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int _screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	/* Calculates the required size of the window-rectangle, based on the desired client-rectangle size. */
	AdjustWindowRect(&_windowRect, WS_OVERLAPPEDWINDOW, FALSE);
	
	/* Measured required window size for desired client size.*/
	auto _windowWidth = _windowRect.right - _windowRect.left;
	auto _windowHeight = _windowRect.bottom - _windowRect.top;

	/* Create & Display Our Window.*/
	HWND hWnd = CreateWindow(WindowClass,WindowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, _windowWidth, _windowHeight, nullptr, nullptr, HInstance(),nullptr);
	
	/* Reposition the window to be spawned at the centre of the screen.*/
	int _X = (_screenWidth  / 2) - (_windowWidth  / 2);
	int _Y = (_screenHeight / 2) - (_windowHeight / 2);
	MoveWindow(hWnd, _X, _Y, _windowWidth, _windowHeight, FALSE);
	assert(hWnd);

	ShowWindow(hWnd, SW_SHOW);
	return hWnd;
}

LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CLOSE:
		ShouldClose = TRUE;
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(NULL);
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}
