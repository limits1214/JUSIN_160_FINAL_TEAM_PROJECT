#pragma once
#include "Engine_Base.h"
#include "IRenderable.h"

NS_BEGIN(Engine)

class CMapMeshGpuCuller;
class CResStaticModel;
class CResTexture2D;


class ENGINE_DLL CMapMeshInstancingRenderer final : public CEngineBase, public IRenderable
{
public:
	CMapMeshInstancingRenderer(const CMapMeshInstancingRenderer&) = delete;
	CMapMeshInstancingRenderer& operator=(const CMapMeshInstancingRenderer& rhs) = delete;

private:
	CMapMeshInstancingRenderer();
	~CMapMeshInstancingRenderer() override;

private:
	HRESULT Initialize();

public:
	void Update();
	void FrameEnd();
	void ClearTextureCache();
	void EraseTextureCache(const SPtr<CResStaticModel>& model);

public:
	// 인스턴싱 On/Off , 드로우 콜 GUI
	_bool IsInstancingEnabled() { return s_bInstancingEnabled; }
	void SetInstancingEnabled(_bool bEnabled);
	const INSTANCING_STATS& GetInstancingStats() { return s_LastStats; }
	_bool IsDebugBoundsEnabled() { return s_bDebugBoundsEnabled; }
	void SetDebugBoundsEnabled(_bool bEnabled) { s_bDebugBoundsEnabled = bEnabled; }

private:
	void ClearInstancingData(); // 매 프레임 인스턴싱 데이터 clear
	void ClearFrameScratchBuffers();
	void ReleaseInstancingResources(); // 종료할 때 인스턴싱 버퍼 해제

private:
	static constexpr size_t MAPMESH_TEXTURE_COUNT = 4;
	using MAPMESH_TEXTURE_SET = std::array<SPtr<CResTexture2D>, MAPMESH_TEXTURE_COUNT>;
	using MAPMESH_TEXTURE_CACHE = std::unordered_map<SPtr<CResStaticModel>, std::vector<MAPMESH_TEXTURE_SET>>;
	struct DRAW_ITEM
	{
		SPtr<CResStaticModel> model{};
		EMapMeshRenderFeature renderFeature{};
		const std::vector<MAPMESH_TEXTURE_SET>* textureCache = nullptr;
		uint32_t meshIndex = 0;
		uint32_t instanceOffset = 0;
	};

	const std::vector<MAPMESH_TEXTURE_SET>* GetOrCreateMapMeshTextureCache(const SPtr<CResStaticModel>& pModel);
	HRESULT BindMapMeshTextures(ID3D11DeviceContext* pContext, const std::vector<MAPMESH_TEXTURE_SET>& textureCache, uint32_t meshIndex) const;

public:
	HRESULT PushMapObjectInstance(const SPtr<CResStaticModel>& pModel, EMapMeshRenderFeature renderFeature, const MAPMESH_INSTANCE_DATA& instanceData, MAPMESH_OCCLUSION_DATA& occlusionData);
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override; // 이 렌더러가 렌더 큐에 들어가서 인스턴싱 렌더 (대표오브젝트 구조 x)
	bool HasRenderPass(RENDERPASS ePass) const override;

private:
	using MAPOBJECTKEY = std::pair<SPtr<CResStaticModel>, EMapMeshRenderFeature>;
	struct MAPOBJECT_HASH
	{
		size_t operator()(const MAPOBJECTKEY& key) const noexcept
		{
			const size_t modelHash = std::hash<SPtr<CResStaticModel>>{}(key.first);
			const size_t featureHash = std::hash<uint32_t>{}(static_cast<uint32_t>(key.second));

			return modelHash ^ (featureHash << 1);
		}
	};
	std::unordered_map<MAPOBJECTKEY, MAPMESH_INSTANCE_BATCH, MAPOBJECT_HASH> s_InstanceBatches;
	MAPMESH_TEXTURE_CACHE m_MapMeshTextureCache;
	UPtr<CMapMeshGpuCuller> s_pGpuCuller;

	// 하이브리드 SoA
	std::vector<MAPMESH_INSTANCE_DATA> m_Instances;
	std::vector<MAPMESH_OCCLUSION_DATA> m_OcclusionData;
	std::vector<MAPMESH_CULL_META> m_CullMeta;

	std::vector<uint32_t> m_DrawBatchIndices;
	std::vector<D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS> m_IndirectArgs;
	std::vector<DRAW_ITEM> m_DrawItems;

	// 드로우 콜 확인용
	_bool s_bInstancingEnabled = true; // 인스턴싱 On/Off
	_bool s_bDebugBoundsEnabled = false;
	INSTANCING_STATS s_FrameStats { true };
	INSTANCING_STATS s_LastStats { true };


	// RenderFeature별 잦은 Set을 방지하기 위해 // 원래 Draw 인덱스를 Static/Foliage 버킷에 따로 저장
	static constexpr size_t RENDER_FEATURE_COUNT = 2;
	std::array<std::vector<uint32_t>, RENDER_FEATURE_COUNT> m_DrawIndicesByFeature;

public:
	static UPtr<CMapMeshInstancingRenderer> Create();

public:
	void Free() override;
};

NS_END
