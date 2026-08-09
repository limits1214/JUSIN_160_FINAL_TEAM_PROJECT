#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CComPxConvexCollider;
class CComPxDistanceJoint;
class CComPxFixedJoint;
class CComPxRigidBody;
class CComStaticModelInstance;
class CResPixelShader;
class CResVertexShader;
NS_END

NS_BEGIN(Client)

class COilBarrel final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(COilBarrel, CGameObject)

	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vInitialPosition{};
		_float3 vInitialRotation{};
		_float3 vInitialScale{ 1.f, 1.f, 1.f };
		_float3 vConvexScale{ 1.f, 1.f, 1.f };
		_float fMass{ 1.f };
		PX_FILTER_DESC tFilter{};
	};

	struct POOL_ACQUIRE_DESC
	{
		_float3 vPosition{};
		_float4 vRotation{ 0.f, 0.f, 0.f, 1.f };
	};

private:
	COilBarrel();
	COilBarrel(const COilBarrel& prototype);
	~COilBarrel() override = default;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	_bool ApplyPushForce(const _float3& vDirection, _float fStrength);
	_bool CreateFixedJoint(
		COilBarrel* pConnectedBarrel = nullptr,
		uint32_t iJointSubIndex = std::numeric_limits<uint32_t>::max());
	_bool CreateDistanceJoint(
		COilBarrel* pConnectedBarrel,
		_float fMaxDistance,
		uint32_t iJointSubIndex = std::numeric_limits<uint32_t>::max());
	CComPxRigidBody* GetRigidBody() const { return m_pComPxRigidBody; }
	void OnJointBreak(const PX_ON_JOINT_BREAK_DATA& tData) override;

	/*----------- 광윤 추가 -----------*/
	HRESULT Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	bool GetShadowBounds(BoundingBox& OutBounds) const override;
	/*---------------------------------*/
public:
	static E::UPtr<COilBarrel> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	CComStaticModelInstance* m_pComModelInstance{};
	CComConstantBuffer* m_pComCBufferPerObject{};
	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxConvexCollider* m_pComPxConvexCollider{};
	CComPxFixedJoint* m_pComPxFixedJoint{};
	CComPxDistanceJoint* m_pComPxDistanceJoint{};
	SPtr<CResVertexShader> m_pResVertexShader{};
	SPtr<CResPixelShader> m_pResPixelShader{};

protected:
	_bool OnAcquireFromPool(void* pArg) override;
	void OnReleaseToPool() override;
	void OnManagedUpdateEnabled() override;
	void OnManagedUpdateDisabled() override;
};

NS_END
