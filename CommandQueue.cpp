#include "CommandQueue.h"

CommandQueue::CommandQueue(Microsoft::WRL::ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type) : m_FenceValue{ NULL }, m_CommandListType{ type }, m_d3d12Device{ device }
{
	D3D12_COMMAND_QUEUE_DESC desc{};
	desc.Type = type;
	ThrowIfFailed(m_d3d12Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_d3d12CommandQueue)));
	ThrowIfFailed(m_d3d12Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12Fence)));
	m_FenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(m_FenceEvent && "Failed to create fence event handle.");
}

CommandQueue::~CommandQueue()
{
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::GetCommandList()
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList;
	
	if (!m_CommandAllocatorQueue.empty() && IsFenceComplete(m_CommandAllocatorQueue.front().fenceValue))
	{
		_commandAllocator = m_CommandAllocatorQueue.front().commandAllocator;
		m_CommandAllocatorQueue.pop();

		ThrowIfFailed(_commandAllocator->Reset());
	}
	else
	{
		_commandAllocator = CreateCommandAllocator();
	}
	if (!m_CommandListQueue.empty())
	{
		_commandList = m_CommandListQueue.front();
		m_CommandListQueue.pop();

		ThrowIfFailed(_commandList->Reset(_commandAllocator.Get(), nullptr));
	}
	else
	{
		_commandList = CreateCommandList(_commandAllocator);
	}
	// Associate the command allocator with the command list so that it can be
	// retrieved when the command list is executed.
	ThrowIfFailed(_commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), _commandAllocator.Get()));
	return _commandList;
}

uint64_t CommandQueue::ExecuteCommandList(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList)
{
	commandList->Close();
	ID3D12CommandAllocator* commandAllocator;
	UINT dataSize = sizeof(commandAllocator);
	ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));
	ID3D12CommandList* const ppCommandLists[] = {
		commandList.Get()
	};
	m_d3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fenceValue = Signal();
	m_CommandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
	m_CommandListQueue.push(commandList);
	// The ownership of the command allocator has been transferred to the ComPtr
	// in the command allocator queue. It is safe to release the reference 
	// in this temporary COM pointer here.
	commandAllocator->Release();

	return fenceValue;
}

uint64_t CommandQueue::Signal()
{
	return 0;
}

bool CommandQueue::IsFenceComplete(uint64_t fenceValue)
{
	return false;
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue)
{
}

void CommandQueue::Flush()
{
}

Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue::GetD3D12CommandQueue() const
{
	return Microsoft::WRL::ComPtr<ID3D12CommandQueue>();
}

Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator()
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _commandAllocator;
	ThrowIfFailed(m_d3d12Device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&_commandAllocator)));
	return _commandAllocator;
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList(Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator)
{
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> _commandList;
	ThrowIfFailed(m_d3d12Device->CreateCommandList(NULL, m_CommandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&_commandList)));
	return _commandList;
}
