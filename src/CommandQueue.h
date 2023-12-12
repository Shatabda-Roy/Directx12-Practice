#pragma once
#include <d3d12.h>
#include <wrl.h>  

#include <cstdint>
#include <queue>
#include <assert.h>
class CommandQueue
{
public:
	CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	virtual ~CommandQueue();
	// Get an available command list from the command queue.
	// The command list returned from this method will be in a state that it can immediately be used to issue commands
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> GetCommandList();
	// Execute a command list.
	// Returns the fence value to wait for for this command list.
	uint64_t ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList);
	uint64_t Signal();
	bool IsFenceComplete(uint64_t fenceValue);
	void WaitForFenceValue(uint64_t fenceValue);
	void Flush();

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;

protected:
	/* used to create a command allocator and command list if no command list or command allocator are currently available.*/
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator);
private:
	// Keep track of command allocators that are "in-flight"
	struct CommandAllocatorEntry
	{
		uint64_t fenceValue;
		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	};

	D3D12_COMMAND_LIST_TYPE                     m_CommandListType;
	Microsoft::WRL::ComPtr<ID3D12Device2>       m_d3d12Device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue>  m_d3d12CommandQueue;
	Microsoft::WRL::ComPtr<ID3D12Fence>         m_d3d12Fence;
	HANDLE                                      m_FenceEvent;
	uint64_t                                    m_FenceValue;
	using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
	/* Object that is used to queue command allocators that are “in - flight” on the GPU queue.
	As soon as the fence value that is associated with the CommandAllocatorEntry has been reached, the command allocator can be reused.
	*/
	CommandAllocatorQueue                       m_CommandAllocatorQueue;
	using CommandListQueue = std::queue<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2>>;
	/* Used to queue command lists that can be reused*/
	CommandListQueue                            m_CommandListQueue;
};