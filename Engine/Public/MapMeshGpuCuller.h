#pragma once
#include "Engine_Base.h"

NS_BEGIN(Engine)

class CResStructuredBuffer;
class CResDynamicBuffer;
class CResStaticModel;
class CHizBuffer;

struct CB_MAPMESH_GPU_CULL
{
	_float4x4 matViewProj{};
	_float2 screenSize{};
	_float2 hizSize{};
	uint32_t instanceCount;
	uint32_t mipCount;
	uint32_t useHiz;
	_float hizBias = 0.0005f;
};

class ENGINE_DLL CMapMeshGpuCuller : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CMapMeshGpuCuller, CEngineBase)

private:
	CMapMeshGpuCuller() = default;
	~CMapMeshGpuCuller() override = default;

public:
	HRESULT EnsureCapacity(uint32_t instanceCount);

	HRESULT BuildVisibleInstances(
		ID3D11DeviceContext* pContext,
		const std::vector<MAPMESH_INSTANCE_DATA>& instances,
		const std::vector<MAPMESH_OCCLUSION_DATA>& occlusionData,
		const CHizBuffer* pPrevHizBuffer,
		_matrix matViewProj,
		const _float2& screenSize);

	HRESULT BuildSubMeshVisibility(
		ID3D11DeviceContext* pContext,
		const std::vector<MAPMESH_OCCLUSION_DATA>& occlusionData,
		const CHizBuffer* pPrevHizBuffer,
		_matrix matViewProj,
		const _float2& screenSize,
		std::vector<uint32_t>& visibility);


	ID3D11Buffer* GetVisibleInstanceBuffer() const;
	ID3D11Buffer* GetIndirectArgsBuffer() const;
	HRESULT PrepareIndirectArgs(
		ID3D11DeviceContext* pContext,
		uint32_t indexCountPerInstance,
		uint32_t startIndexLocation = 0,
		int32_t baseVertexLocation = 0,
		uint32_t startInstanceLocation = 0);

	// 디버그 확인용!! cpu readback이라 병목 발생
	uint32_t GetVisibleCountForDebug(ID3D11DeviceContext* pContext);
private:
	uint32_t m_iCapacity = 0;

	// MAPMESH_INSTANCE_DATA[] // 각자의 월드행렬
	SPtr<CResStructuredBuffer> m_pInstanceInputBuffer{};

	// MAPMESH_OCCLUSION_DATA[] // 각자의 boundingBox
	SPtr<CResStructuredBuffer> m_pOcclusionInputBuffer{};

	// MAPMESH_INSTANCE_DATA[] // ComputeShader가 보이는 instance만 씀
	SPtr<CResStructuredBuffer> m_pVisibleInstanceBuffer{};

	// Vertex shader에서 instance buffer로 사용
	ComPtr<ID3D11Buffer> m_pVisibleInstanceVertexBuffer{};

	// DrawIndexedInstancedIndirect 인자
	ComPtr<ID3D11Buffer> m_pIndirectArgsBuffer{};
	
	// 디버그 확인용으로 cpu readback 하기위해 gpu에서 복사해올 버퍼
	ComPtr<ID3D11Buffer> m_pVisibleCountStagingBuffer{};

	SPtr<CResStructuredBuffer> m_pVisibilityFlagBuffer{};
	static constexpr uint32_t SUBMESH_READBACK_SLOT_COUNT = 3;
	struct SUBMESH_READBACK_SLOT
	{
		ComPtr<ID3D11Buffer> buffer{};
		uint32_t elementCount = 0;
		bool pending = false;
	};
	SUBMESH_READBACK_SLOT m_SubMeshReadbackSlots[SUBMESH_READBACK_SLOT_COUNT]{};
	uint32_t m_iNextSubMeshReadbackSlot = 0;

	ComPtr<ID3D11Buffer> m_pCBuffer{}; // ComputeShader가 instanceCount를 알아야 함

public:
	static UPtr<CMapMeshGpuCuller> Create();
};

NS_END
