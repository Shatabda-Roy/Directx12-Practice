#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include <memory>
#include <string>
class AppWindow;
class App
{
public:
    /*Create the application singleton with the application instance handle. */
    static void Create(HINSTANCE hInst);
    /*Destroy the application instance and all windows created by this application instance.*/
    static void Destroy();
    /*Get the application singleton.*/
    static App& Get();
    /*Check to see if VSync-off is supported.*/
    bool IsTearingSupported() const;
    
    std::shared_ptr<AppWindow> CreateRenderWindow(const WCHAR windowName, int clientWidth, int clientHeight, bool vSync = true);

    /*Destroy a window given the window name.*/
    void DestroyWindow(const std::wstring& windowName);
    /*Destroy a window given the window reference.*/
    void DestroyWindow(std::shared_ptr<AppWindow> window);

    /*Find a window by the window name.*/
    std::shared_ptr<AppWindow> GetWindowByName(const std::wstring& windowName);

    /*Run the application loop and message pump. @return The error code if an error occurred.*/
    int Run();

    /*Request to quit the application and close all windows.@param exitCode The error code to return to the invoking process. */
    void Quit(int exitCode = 0);

    /*Get the Direct3D 12 device*/
    Microsoft::WRL::ComPtr<ID3D12Device2> GetDevice() const;
    
    //std::shared_ptr<CommandQueue> GetCommandQueue(D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT) const;

    // Flush all command queues.
    void Flush();

    // Flush all command queues.
    void Flush();

    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type);
    UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;

};

