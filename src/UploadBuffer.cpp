#include "UploadBuffer.h"
#include "Helpers.h"
#include <new> // for std::bad_alloc

UploadBuffer::UploadBuffer(size_t pageSize, Microsoft::WRL::ComPtr<ID3D12Device2> device) : m_PageSize{pageSize},m_Device{device}
{
}

UploadBuffer::~UploadBuffer()
{
}

UploadBuffer::Allocation UploadBuffer::Allocate(size_t sizeInBytes, size_t alignment)
{
	if (sizeInBytes > m_PageSize) {
		throw std::bad_alloc();
	}
	if (!m_CurrentPage || !m_CurrentPage->HasSpace(sizeInBytes, alignment)) {
		m_CurrentPage = RequestPage();
	}
	return m_CurrentPage->Allocate(sizeInBytes, alignment);
}

void UploadBuffer::Reset()
{
	m_CurrentPage = nullptr;
	m_AvailablePages = m_PagePool;
	for (auto page : m_AvailablePages) {
		page->Reset();
	}
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
{
	std::shared_ptr<Page> page;
	if (!m_AvailablePages.empty()) {
		page = m_AvailablePages.front();
		m_AvailablePages.pop_front();
	}
	else {
		page = std::make_shared<Page>(m_PageSize);
		m_PagePool.push_back(page);
	}
	return page;
}

UploadBuffer::Page::Page(size_t sizeInBytes) : m_PageSize{sizeInBytes}, m_Offset{NULL},m_CPUPtr{nullptr},m_GPUPtr{D3D12_GPU_VIRTUAL_ADDRESS(NULL)}
{
}

UploadBuffer::Page::~Page()
{
}

bool UploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
	return false;
}

UploadBuffer::Allocation UploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
	return Allocation();
}

void UploadBuffer::Page::Reset()
{
}
