#include "pch.h"
#include "MapMeshGpuCuller.h"
#include "HizBuffer.h"

NS_USING(Engine)

HRESULT CMapMeshGpuCuller::EnsureCapacity(uint32_t instanceCount)
{
	if (instanceCount == 0)
		return S_OK;

	if (m_iCapacity >= instanceCount &&
		m_pInstanceInputBuffer &&
		m_pOcclusionInputBuffer &&
		m_pVisibleInstanceBuffer &&
		m_pVisibilityFlagBuffer &&
		m_pVisibleInstanceVertexBuffer &&
		m_pIndirectArgsBuffer &&
		m_pVisibleCountStagingBuffer &&
		m_pCBuffer)
	{
		return S_OK;
	}

	size_t newCapacity = std::max<size_t>(instanceCount, 256);
	while (newCapacity < instanceCount)
	{
		newCapacity *= 2;
	}


	{
		auto pInstanceInputBuffer = CResStructuredBuffer::Create();
		if (pInstanceInputBuffer == nullptr)
		{
			return E_FAIL;
		}

		CResStructuredBuffer::DESC bufferDesc{};
		bufferDesc.iNumElements = static_cast<uint32_t>(newCapacity);
		bufferDesc.iStructureByteStride = sizeof(MAPMESH_INSTANCE_DATA);
		bufferDesc.pInitialData = nullptr;
		bufferDesc.bAppendConsume = false;
		bufferDesc.iBindFlags = D3D11_BIND_SHADER_RESOURCE;
		if (FAILED(pInstanceInputBuffer->Load(bufferDesc)))
		{
			return E_FAIL;
		}

		m_pInstanceInputBuffer = pInstanceInputBuffer;
	}

	{
		auto pOcclusionInputBuffer = CResStructuredBuffer::Create();
		if (pOcclusionInputBuffer == nullptr)
		{
			return E_FAIL;
		}

		CResStructuredBuffer::DESC bufferDesc{};
		bufferDesc.iNumElements = static_cast<uint32_t>(newCapacity);
		bufferDesc.iStructureByteStride = sizeof(MAPMESH_OCCLUSION_DATA);
		bufferDesc.pInitialData = nullptr;
		bufferDesc.bAppendConsume = false;
		bufferDesc.iBindFlags = D3D11_BIND_SHADER_RESOURCE;
		if (FAILED(pOcclusionInputBuffer->Load(bufferDesc)))
		{
			return E_FAIL;
		}

		m_pOcclusionInputBuffer = pOcclusionInputBuffer;
	}

	{
		auto pVisibleInstanceBuffer = CResStructuredBuffer::Create();
		if (pVisibleInstanceBuffer == nullptr)
		{
			return E_FAIL;
		}

		CResStructuredBuffer::DESC bufferDesc{};
		bufferDesc.iNumElements = static_cast<uint32_t>(newCapacity);
		bufferDesc.iStructureByteStride = sizeof(MAPMESH_INSTANCE_DATA);
		bufferDesc.pInitialData = nullptr;
		bufferDesc.bAppendConsume = true;
		bufferDesc.iBindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		if (FAILED(pVisibleInstanceBuffer->Load(bufferDesc)))
		{
			return E_FAIL;
		}

		m_pVisibleInstanceBuffer = pVisibleInstanceBuffer;
	}

	{
		auto pVisibilityFlagBuffer = CResStructuredBuffer::Create();
		if (pVisibilityFlagBuffer == nullptr)
		{
			return E_FAIL;
		}

		CResStructuredBuffer::DESC bufferDesc{};
		bufferDesc.iNumElements = static_cast<uint32_t>(newCapacity);
		bufferDesc.iStructureByteStride = sizeof(uint32_t);
		bufferDesc.pInitialData = nullptr;
		bufferDesc.bAppendConsume = false;
		bufferDesc.iBindFlags = D3D11_BIND_UNORDERED_ACCESS;
		if (FAILED(pVisibilityFlagBuffer->Load(bufferDesc)))
		{
			return E_FAIL;
		}

		m_pVisibilityFlagBuffer = pVisibilityFlagBuffer;
	}

	{
		D3D11_BUFFER_DESC bufferDesc{};
		bufferDesc.ByteWidth = static_cast<UINT>(sizeof(MAPMESH_INSTANCE_DATA) * newCapacity);
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = 0;
		bufferDesc.StructureByteStride = sizeof(MAPMESH_INSTANCE_DATA);

		if (FAILED(CGameInstance::Get().GetGraphicDevice()->CreateBuffer(&bufferDesc, nullptr, m_pVisibleInstanceVertexBuffer.ReleaseAndGetAddressOf())))
		{
			return E_FAIL;
		}
	}

	if (m_pIndirectArgsBuffer == nullptr)
	{
		D3D11_BUFFER_DESC bufferDesc{};
		bufferDesc.ByteWidth = sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS);
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		bufferDesc.BindFlags = 0;
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;

		if (FAILED(CGameInstance::Get().GetGraphicDevice()->CreateBuffer(&bufferDesc, nullptr, m_pIndirectArgsBuffer.GetAddressOf())))
		{
			return E_FAIL;
		}
	}

	// 디버그 확인용
	if (m_pVisibleCountStagingBuffer == nullptr)
	{
		D3D11_BUFFER_DESC bufferDesc{};
		bufferDesc.ByteWidth = sizeof(uint32_t);
		bufferDesc.Usage = D3D11_USAGE_STAGING;
		bufferDesc.BindFlags = 0;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		bufferDesc.MiscFlags = 0;

		if (FAILED(CGameInstance::Get().GetGraphicDevice()->CreateBuffer(&bufferDesc, nullptr, m_pVisibleCountStagingBuffer.GetAddressOf())))
		{
			return E_FAIL;
		}
	}

	if (m_pCBuffer == nullptr)
	{
		D3D11_BUFFER_DESC bufferDesc{};
		bufferDesc.ByteWidth = sizeof(CB_MAPMESH_GPU_CULL);
		bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		if (FAILED(CGameInstance::Get().GetGraphicDevice()->CreateBuffer(&bufferDesc, nullptr, m_pCBuffer.GetAddressOf())))
		{
			return E_FAIL;
		}
	}

	for (auto& slot : m_SubMeshReadbackSlots)
	{
		D3D11_BUFFER_DESC bufferDesc{};
		bufferDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * newCapacity);
		bufferDesc.Usage = D3D11_USAGE_STAGING;
		bufferDesc.BindFlags = 0;
		bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
		bufferDesc.MiscFlags = 0;

		slot.buffer.Reset();
		slot.elementCount = 0;
		slot.pending = false;
		if (FAILED(CGameInstance::Get().GetGraphicDevice()->CreateBuffer(
			&bufferDesc, nullptr, slot.buffer.GetAddressOf())))
		{
			return E_FAIL;
		}
	}
	m_iNextSubMeshReadbackSlot = 0;

	m_iCapacity = static_cast<uint32_t>(newCapacity);

	return S_OK;
}

HRESULT CMapMeshGpuCuller::BuildSubMeshVisibility(
	ID3D11DeviceContext* pContext,
	const std::vector<MAPMESH_OCCLUSION_DATA>& occlusionData,
	const CHizBuffer* pPrevHizBuffer,
	_matrix matViewProj,
	const _float2& screenSize,
	std::vector<uint32_t>& visibility)
{
	if (pContext == nullptr || occlusionData.empty())
		return E_FAIL;

	const uint32_t elementCount = static_cast<uint32_t>(occlusionData.size());
	if (FAILED(EnsureCapacity(elementCount)))
		return E_FAIL;

	for (uint32_t i = 0; i < SUBMESH_READBACK_SLOT_COUNT; ++i)
	{
		const uint32_t slotIndex =
			(m_iNextSubMeshReadbackSlot + i) % SUBMESH_READBACK_SLOT_COUNT;
		auto& slot = m_SubMeshReadbackSlots[slotIndex];
		if (!slot.pending)
			continue;

		D3D11_MAPPED_SUBRESOURCE mapped{};
		const HRESULT mapResult = pContext->Map(
			slot.buffer.Get(), 0, D3D11_MAP_READ, D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped);
		if (mapResult == DXGI_ERROR_WAS_STILL_DRAWING)
			continue;
		if (FAILED(mapResult))
			return mapResult;

		visibility.resize(slot.elementCount);
		memcpy(visibility.data(), mapped.pData, sizeof(uint32_t) * slot.elementCount);
		pContext->Unmap(slot.buffer.Get(), 0);
		slot.pending = false;
		break;
	}

	if (FAILED(m_pOcclusionInputBuffer->UpdateData(
		occlusionData.data(),
		static_cast<uint32_t>(sizeof(MAPMESH_OCCLUSION_DATA) * elementCount))))
	{
		return E_FAIL;
	}

	SPtr<CResComputeShader> shader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(
		TAG_RES_GRP_PERMANENT_SHADER, "CS_SubMeshVisibility");
	if (shader == nullptr)
		return E_FAIL;

	pContext->CSSetShader(shader->GetComputeShader().Get(), nullptr, 0);

	ComPtr<ID3D11ShaderResourceView> prevHizSRV =
		pPrevHizBuffer ? pPrevHizBuffer->GetSRV() : nullptr;
	ID3D11ShaderResourceView* srvs[] = {
		m_pOcclusionInputBuffer->GetSRV().Get(),
		prevHizSRV.Get()
	};
	pContext->CSSetShaderResources(0, 2, srvs);

	ID3D11UnorderedAccessView* uavs[] = { m_pVisibilityFlagBuffer->GetUAV().Get() };
	pContext->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

	CB_MAPMESH_GPU_CULL cb{};
	XMStoreFloat4x4(&cb.matViewProj, matViewProj);
	cb.screenSize = screenSize;
	cb.hizSize = pPrevHizBuffer
		? _float2{ static_cast<_float>(pPrevHizBuffer->GetWidth()), static_cast<_float>(pPrevHizBuffer->GetHeight()) }
		: _float2{};
	cb.instanceCount = elementCount;
	cb.mipCount = pPrevHizBuffer ? pPrevHizBuffer->GetMipCount() : 0;
	cb.useHiz = (pPrevHizBuffer != nullptr && prevHizSRV != nullptr &&
		cb.mipCount > 0 && screenSize.x > 0.f && screenSize.y > 0.f) ? 1u : 0u;

	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(pContext->Map(m_pCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		return E_FAIL;
	memcpy(mapped.pData, &cb, sizeof(cb));
	pContext->Unmap(m_pCBuffer.Get(), 0);
	pContext->CSSetConstantBuffers(0, 1, m_pCBuffer.GetAddressOf());

	pContext->Dispatch((elementCount + 63) / 64, 1, 1);

	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };
	ID3D11Buffer* nullCBuffers[] = { nullptr };
	pContext->CSSetShaderResources(0, 2, nullSRVs);
	pContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	pContext->CSSetConstantBuffers(0, 1, nullCBuffers);
	pContext->CSSetShader(nullptr, nullptr, 0);

	auto& copySlot = m_SubMeshReadbackSlots[m_iNextSubMeshReadbackSlot];
	if (!copySlot.pending)
	{
		pContext->CopyResource(copySlot.buffer.Get(), m_pVisibilityFlagBuffer->GetBuffer().Get());
		copySlot.elementCount = elementCount;
		copySlot.pending = true;
		m_iNextSubMeshReadbackSlot =
			(m_iNextSubMeshReadbackSlot + 1) % SUBMESH_READBACK_SLOT_COUNT;
	}

	return S_OK;
}

HRESULT CMapMeshGpuCuller::BuildVisibleInstances(
	ID3D11DeviceContext* pContext,
	const std::vector<MAPMESH_INSTANCE_DATA>& instances,
	const std::vector<MAPMESH_OCCLUSION_DATA>& occlusionData,
	const CHizBuffer* pPrevHizBuffer,
	_matrix matViewProj,
	const _float2& screenSize)
{
	if (pContext == nullptr || instances.empty())
		return E_FAIL;

	const uint32_t instanceCount = static_cast<uint32_t>(instances.size());
	const bool hasOcclusionData = occlusionData.size() == instances.size();

	if (FAILED(EnsureCapacity(instanceCount)))
		return E_FAIL;

	if (FAILED(m_pInstanceInputBuffer->UpdateData(instances.data(), static_cast<uint32_t>(sizeof(MAPMESH_INSTANCE_DATA) * instances.size()))))
		return E_FAIL;

	if (!occlusionData.empty())
	{
		if (!hasOcclusionData)
		{
			return E_FAIL;
		}

		if (FAILED(m_pOcclusionInputBuffer->UpdateData(occlusionData.data(), static_cast<uint32_t>(sizeof(MAPMESH_OCCLUSION_DATA) * occlusionData.size()))))
		{
			return E_FAIL;
		}
	}

	SPtr<CResComputeShader> shader = CGameInstance::Get().GetResourceFirst<CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_MapMeshGpuCull");
	if (shader == nullptr)
		return E_FAIL;


	pContext->CSSetShader(shader->GetComputeShader().Get(), nullptr, 0);

	ComPtr<ID3D11ShaderResourceView> prevHizSRV = pPrevHizBuffer ? pPrevHizBuffer->GetSRV() : nullptr;
	ID3D11ShaderResourceView* srvs[] = {
		m_pInstanceInputBuffer->GetSRV().Get(),
		m_pOcclusionInputBuffer->GetSRV().Get(),
		prevHizSRV.Get()
	};
	pContext->CSSetShaderResources(0, 3, srvs);

	ID3D11UnorderedAccessView* uavs[] = { m_pVisibleInstanceBuffer->GetUAV().Get() };
	UINT initialCounts[] = { 0 };
	pContext->CSSetUnorderedAccessViews(0, 1, uavs, initialCounts);

	CB_MAPMESH_GPU_CULL cb{};
	XMStoreFloat4x4(&cb.matViewProj, matViewProj);
	cb.screenSize = screenSize;
	cb.hizSize = pPrevHizBuffer ? _float2{ static_cast<_float>(pPrevHizBuffer->GetWidth()), static_cast<_float>(pPrevHizBuffer->GetHeight()) } : _float2{};
	cb.instanceCount = instanceCount;
	cb.mipCount = pPrevHizBuffer ? pPrevHizBuffer->GetMipCount() : 0;
	cb.useHiz = (hasOcclusionData && pPrevHizBuffer != nullptr && prevHizSRV != nullptr && cb.mipCount > 0 && screenSize.x > 0.f && screenSize.y > 0.f) ? 1u : 0u;
	D3D11_MAPPED_SUBRESOURCE mappedSubResource;
	if (FAILED(pContext->Map(m_pCBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
	{
		return E_FAIL;
	}
	memcpy(mappedSubResource.pData, &cb, sizeof(CB_MAPMESH_GPU_CULL));
	pContext->Unmap(m_pCBuffer.Get(), 0);
	pContext->CSSetConstantBuffers(0, 1, m_pCBuffer.GetAddressOf());

	const uint32_t groupX = (instanceCount + 63) / 64;
	pContext->Dispatch(groupX, 1, 1);


	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr, nullptr };
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };
	pContext->CSSetShaderResources(0, 3, nullSRVs);
	pContext->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
	ID3D11Buffer* nullCBuffers[] = { nullptr };
	pContext->CSSetConstantBuffers(0, 1, nullCBuffers);
	pContext->CSSetShader(nullptr, nullptr, 0);

	// AppendStructuredBuffer로 visible 인스턴스만 써둔 gpu버퍼를 복사 // gpu-gpu내 메모리 복사
	pContext->CopyResource(m_pVisibleInstanceVertexBuffer.Get(), m_pVisibleInstanceBuffer->GetBuffer().Get());

	return S_OK;
}

HRESULT CMapMeshGpuCuller::PrepareIndirectArgs(
	ID3D11DeviceContext* pContext,
	uint32_t indexCountPerInstance,
	uint32_t startIndexLocation,
	int32_t baseVertexLocation,
	uint32_t startInstanceLocation)
{
	if (pContext == nullptr || m_pIndirectArgsBuffer == nullptr || m_pVisibleInstanceBuffer == nullptr)
	{
		return E_FAIL;
	}

	D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS args{};
	args.IndexCountPerInstance = indexCountPerInstance;
	args.InstanceCount = 0;
	args.StartIndexLocation = startIndexLocation;
	args.BaseVertexLocation = baseVertexLocation;
	args.StartInstanceLocation = startInstanceLocation;

	pContext->UpdateSubresource(m_pIndirectArgsBuffer.Get(), 0, nullptr, &args, 0, 0);

	// offset 4바이트 줘서 args.InstanceCount 복사해옴
	// cpu readback하는게 아니라 gpu내부에서 uav가 관리하는 내부카운터를 복사해오는것.
	pContext->CopyStructureCount(m_pIndirectArgsBuffer.Get(), sizeof(uint32_t), m_pVisibleInstanceBuffer->GetUAV().Get());


	return S_OK;
}

uint32_t CMapMeshGpuCuller::GetVisibleCountForDebug(ID3D11DeviceContext* pContext)
{
	if (pContext == nullptr || m_pVisibleCountStagingBuffer == nullptr || m_pVisibleInstanceBuffer == nullptr)
	{
		return 0;
	}

	pContext->CopyStructureCount(m_pVisibleCountStagingBuffer.Get(), 0, m_pVisibleInstanceBuffer->GetUAV().Get());

	D3D11_MAPPED_SUBRESOURCE mapped{};
	if (FAILED(pContext->Map(m_pVisibleCountStagingBuffer.Get(), 0, D3D11_MAP_READ, 0, &mapped)))
	{
		return 0;
	}

	const uint32_t visibleCount = *static_cast<const uint32_t*>(mapped.pData);
	pContext->Unmap(m_pVisibleCountStagingBuffer.Get(), 0);

	return visibleCount;
}

ID3D11Buffer* CMapMeshGpuCuller::GetVisibleInstanceBuffer() const
{
	return m_pVisibleInstanceVertexBuffer.Get();
}

ID3D11Buffer* CMapMeshGpuCuller::GetIndirectArgsBuffer() const
{
	return m_pIndirectArgsBuffer.Get();
}


UPtr<CMapMeshGpuCuller> CMapMeshGpuCuller::Create()
{
	return ToUPtr(new CMapMeshGpuCuller);
}
