#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Client)

class CMonEffectBall : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CMonEffectBall, CGameObject)

public:
	typedef struct tag_Mon_Ball : public CGameObject::GAMEOBJECT_DESC
	{
		_float fDamage{};
		CHandle hOwner{}, hTarget{};
		uint32_t iBoneIndex{};
		PX_QUERY_FILTER_DESC tQueryFilter{
					.iQueryMask = ETOUI(COLLISION_LAYER::PLAYER_HURTBOX) | ETOUI(COLLISION_LAYER::WORLD_STATIC) ,
					.bQueryStatic = true,
					.bQueryDynamic = true,
					.bIncludeTrigger = false
		};
	}MON_BALL;

private:
	CMonEffectBall();
	~CMonEffectBall() override;

public:
	void UpdateGUI() override;
public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void FixedUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

	void      Reset_Active() { m_bHit = false; }
private:
	void      OverlapTest();
	void      Chase(_float fTimeDelta);
private:
	int32_t		m_iDamage{};
	_float		m_fDeadTime{};
	_float3		m_vDir{}, m_vEndLook{}, m_vPos{};
	_bool		m_bPatternBroken{ false };
	_float		m_fSpeed{150.f}, m_fPower{8.f};
	_float4x4	m_CurWorldmat{}, m_Offsetmat{};
	uint32_t	m_iBoneIndex{}, m_iEffectID{};
	CHandle		m_hParent{}, m_hTarget{};
	PX_QUERY_FILTER_DESC m_tQueryFilter{};

	_bool      m_bHit{ false }, m_bThrow{ false };
public:
	static E::UPtr<CMonEffectBall> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
