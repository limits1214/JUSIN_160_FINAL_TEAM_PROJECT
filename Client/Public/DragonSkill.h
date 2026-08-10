#pragma once
#include "Client_Defines.h"
#include "EnderDragon.h"

//찾아라 드래곤볼

NS_BEGIN(Client)
class CDragonSkill abstract : public CGameObject
{
public:
	typedef struct edgskill : public CGameObject::GAMEOBJECT_DESC
	{
		CHandle hOwner;
		int32_t iBoneIndex{ -1 }, iOffsetBoneIndex{};
		PX_QUERY_FILTER_DESC tQueryFilter{ .iQueryMask =
			ETOUI(COLLISION_LAYER::PLAYER_HURTBOX) | ETOUI(COLLISION_LAYER::WORLD_STATIC),
		.bQueryStatic = true,
		.bQueryDynamic = true,
		.bIncludeTrigger = false };
	}EDG_SKILL_DESC;
public:
	DECLARE_DERIVED_TYPE(CDragonSkill, CGameObject)

protected:
	explicit CDragonSkill();
	explicit CDragonSkill(const CDragonSkill& rhs);
	~CDragonSkill() override;

public:
	HRESULT						InitializePrototype(void* pArg = nullptr) override;
	HRESULT						Initialize(void* pArg) override;
	void						PriorityUpdate(E::_float fTimeDelta) override;
	void						FixedUpdate(E::_float fTimeDelta) override;
	void						Update(E::_float fTimeDelta) override;
	void						LateUpdate(E::_float fTimeDelta) override;
public:
	virtual void Active(const _string& SkillName) PURE;
	virtual void Cancle();
protected:
	void						 Spawn_Skill_Effect(const _string& SkillName);
	_bool						 Life_Check(_float fTimeDelta);
	 _float4x4					 Get_BoneMatrix(int32_t iIndex);
	void						 ResetValue();
	CEnderDragon*				 Get_Owner();
	void						 Set_TargetDir(_vector vSrcPos);
	void						 DebugLine(_float3 vPos);
protected:
	CHandle						m_hOwner{};
	
	uint32_t					m_iSkillEffID{};
	int32_t						m_iBoneIndex{ -1 }, m_iOffsetBoneIdex{};

	_float						m_fDamage{}, m_fSpeed{}, m_fRadius{}, m_fLife{}, m_fMaxLife{};
	_float3						m_vDir{}, m_vTargetDir{};
	_bool						m_bActive{ false }, m_bHit{ false }, m_bThrow{false};

	PX_QUERY_FILTER_DESC		m_pxQueryFilter{};
};

NS_END

