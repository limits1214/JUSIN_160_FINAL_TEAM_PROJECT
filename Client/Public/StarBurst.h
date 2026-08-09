#pragma once
#include "GameObject.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CBoss_StarBurst : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CBoss_StarBurst, CGameObject)

public:
	typedef struct tag_StarBurst_desc : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vStartPosition{};
		_float3 vEndPosition{};
		_float  fSpeed{ 10.f };
		_float fRadius{ 0.5f };
		CHandle	pTargetHandle{};
		CHandle hOwner{};
		PX_QUERY_FILTER_DESC tQueryFilter{
			.iQueryMask = ETOUI(COLLISION_LAYER::PLAYER_HURTBOX),
			.bQueryStatic = false,
			.bQueryDynamic = true,
			.bIncludeTrigger = false
		};
	}STARBURST_DESC;

private:
	CBoss_StarBurst();
	~CBoss_StarBurst() override;

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

	void Translate_Casting(_float fRatio);
	void Translate_Attacking(_float fTimeDelta);
	_bool MoveWithSweep(const _float3& vNextPosition);
	_bool HandleSweepHit(const PX_SWEEP_RESULT& tHit);
private:
	void		Dead_Check(_float fTimeDetla);
private:
	_bool  m_bDead{ false };
	_float m_fDeadTick{};
	_float m_fSpeed{ 10.f };
	_float m_fRadius{ 0.5f };
	PX_QUERY_FILTER_DESC m_tQueryFilter{};
	_float m_fPrevEffectMovementValue{};
	_float m_fCurrEffectMovementValue{};
	_float	m_fEffectLifeTime{};
	_float	m_fEffectSpawnTimer{};

	CHandle	m_pTargetHandle{}, m_hOwner{};
	XMVECTOR m_vDirection{};

	EFFECT_INSTANCE_ID	m_pLightEffectID{};

	int32_t				m_iDamage{30};
	_float m_pLavaFlame_SpawnInterval{};
	_float m_pLavaFlame_CastingTime{};
	_float m_pLavaFlame_StayTime{};

public:
	static E::UPtr<CBoss_StarBurst> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
