#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
class CComConstantBuffer;
class CResTexture2D;
class CResVertexShader;
class CResPixelShader;
class CResSamplerState;

class CComPxRigidBody;
class CComPxTriMeshCollider;
class CResPhysXRTTriMeshGeometry;

class CComPxHeightFieldCollider;
class CResPhysXRTHeightFieldGeometry;
NS_END

NS_BEGIN(Client)
class CResTerrainVIBuffer;
class CTerrain final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CTerrain, CGameObject)

private:
	CTerrain();
	~CTerrain() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

private:
	HRESULT BuildPxRuntimeTriMesh();
	HRESULT BuildPxRuntimeHeightField();

private:
	SPtr<CResTerrainVIBuffer> m_pResTerrainVIBuffer{};
	SPtr<CResTexture2D> m_pResTerrainTexture2D{};
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	CComConstantBuffer* m_pComCBufferPerObject{};

private:
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxTriMeshCollider* m_pComPxTriMeshCollider{};
	SPtr<CResPhysXRTTriMeshGeometry> m_pResTriMesh{};

private:
	CComPxHeightFieldCollider* m_pComPxHeightFieldCollider{};
	SPtr<CResPhysXRTHeightFieldGeometry> m_pResHeightField{};
	_float m_fPxHeightScale{ 1.f };
	_float m_fPxRowScale{ 1.f };
	_float m_fPxColumnScale{ 1.f };
	_float3 m_vPxHeightFieldOffset{};


public:
	static E::UPtr<CTerrain> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
