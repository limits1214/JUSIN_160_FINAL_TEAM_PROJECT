#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)

class CComConstantBuffer;
class CComStaticModelInstance;
class CResDynamicBuffer;
class CResPixelShader;
class CResSamplerState;
class CResStaticModel;
class CResStructuredBuffer;
class CResVertexShader;
class CMapMeshGpuCuller;


class ENGINE_DLL CMapMeshObject : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CMapMeshObject, CGameObject)
	CMapMeshObject& operator=(const CMapMeshObject&) = delete;

protected:
	explicit CMapMeshObject();
	explicit CMapMeshObject(const CMapMeshObject& Prototype);
	~CMapMeshObject() override;

public:
	typedef struct tagMapMeshObjectDesc : public GAMEOBJECT_DESC
	{
		std::string modelGroupTag;
		std::string modelResTag;
		std::string protoGroupTag;
		std::string prototypeTag;
	} MAP_MESH_OBJECT_DESC;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	void UpdateGUI() override;

public:
	bool IsOcclusionCullable() const override;
	bool GetOcclusionBounds(BoundingBox& outBounds) const override;
public:
	const std::string& GetModelResourceGroup() const { return m_modelResourceGroup; }
	const std::string& GetModelResourceTag() const { return m_modelResourceTag; }
	HRESULT SetModelResource(const std::string& modelGroupTag, const std::string& modelResTag);

public:
	void SetRenderEnable(_bool enable) { m_bRenderEnable = enable; }

public:
	struct INSTANCING_STATS
	{
		_bool bEnabled = false;
		uint32_t iObjects = 0;
		uint32_t iInstances = 0;
		uint32_t iBatches = 0;
		uint32_t iDrawCalls = 0;
		uint32_t iVisibleInstances = 0;
		uint32_t iCulledInstances = 0;
	};

	// 인스턴싱 On/Off , 드로우 콜 GUI
	static _bool IsInstancingEnabled() { return s_bInstancingEnabled; }
	static void SetInstancingEnabled(_bool bEnabled);
	static const INSTANCING_STATS& GetInstancingStats() { return s_LastStats; }
	static _bool IsDebugBoundsEnabled() { return s_bDebugBoundsEnabled; }
	static void SetDebugBoundsEnabled(_bool bEnabled) { s_bDebugBoundsEnabled = bEnabled; }

	static void ClearInstancingData(); // 매 프레임 인스턴싱 데이터 clear
	static void ReleaseInstancingResources(); // 종료할 때 인스턴싱 버퍼 해제

private:
	static HRESULT PushInstance(const SPtr<CResStaticModel>& pModel, const MAPMESH_INSTANCE_DATA& instanceData, MAPMESH_OCCLUSION_DATA& occlusionData);
	static HRESULT EnsureInstanceResources(size_t instanceCount);
	HRESULT RenderInstancedBatches(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx);

private:
	std::string m_modelResourceGroup{};
	std::string m_modelResourceTag{};
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	UPtr<CMapMeshGpuCuller> m_pSubMeshGpuCuller{};
	std::vector<uint32_t> m_SubMeshVisibility{};

private:
	static std::unordered_map<SPtr<CResStaticModel>, MAPMESH_INSTANCE_BATCH> s_InstanceBatches;
	static SPtr<CResDynamicBuffer> s_pInstanceBuffer;
	static size_t s_iInstanceCapacity;
	static std::optional<CHandle> s_hRenderRepresentative;// 대표로 렌더 콜 호출할 오브젝트
	static UPtr<CMapMeshGpuCuller> s_pGpuCuller;

	static SPtr<CResStructuredBuffer>  s_pOcclusionInputBuffer; // MAPMESH_OCCLUSION_DATA[]
	static SPtr<CResStructuredBuffer> s_pVisibleFlagBuffer; // uint[]

	// 드로우 콜 확인용
	static _bool s_bInstancingEnabled; // 인스턴싱 On/Off
	static _bool s_bDebugBoundsEnabled;
	static INSTANCING_STATS s_FrameStats;
	static INSTANCING_STATS s_LastStats;

private:
	_bool m_bRenderEnable = true; // 렌더러에 들어갈 놈인가 (컬링 통과된 놈들)

	_float3 m_fEmissiveColor		= { 1.f, 1.f, 1.f };		// Emissive 색상. 텍스쳐가 있으면 {1.f, 1.f, 1.f} = 원색
	_float	m_fEmissiveIntensity	= 0.f;						// Emissive 강도,
	_float	m_fObjectAlpha			= 1.f;						// Object의 투명도,

public:
	static UPtr<CMapMeshObject> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
