#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)

class CComConstantBuffer;
class CComStaticModelInstance;
class CResPixelShader;
class CResSamplerState;
class CResStaticModel;
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
		WIND_DESC windDesc;
	} MAP_MESH_OBJECT_DESC;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void LateUpdate(_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	void UpdateGUI() override;

	/*----------- 광윤 추가 -----------*/
	HRESULT Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	bool	GetShadowBounds(BoundingBox& OutBounds) const override;
	/*---------------------------------*/

public:
	bool IsOcclusionCullable() const override;
	bool GetOcclusionBounds(BoundingBox& outBounds) const override;
public:
	const std::string& GetModelResourceGroup() const { return m_modelResourceGroup; }
	const std::string& GetModelResourceTag() const { return m_modelResourceTag; }
	HRESULT SetModelResource(const std::string& modelGroupTag, const std::string& modelResTag);
	const WIND_DESC& GetWindDesc() const { return m_WindDesc; }
	void SetWindDesc(const WIND_DESC& windDesc) { m_WindDesc = windDesc; }

public:
	void SetRenderEnable(_bool enable) { m_bRenderEnable = enable; }

private:
	std::string m_modelResourceGroup{};
	std::string m_modelResourceTag{};
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};

private:
	_bool m_bRenderEnable = true; // 렌더러에 들어갈 놈인가 (컬링 통과된 놈들)

	_float3 m_fEmissiveColor		= { 1.f, 1.f, 1.f };		// Emissive 색상. 텍스쳐가 있으면 {1.f, 1.f, 1.f} = 원색
	_float	m_fEmissiveIntensity	= 0.f;						// Emissive 강도,
	_float	m_fObjectAlpha			= 1.f;						// Object의 투명도,

private:
	WIND_DESC m_WindDesc {};

public:
	static UPtr<CMapMeshObject> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
