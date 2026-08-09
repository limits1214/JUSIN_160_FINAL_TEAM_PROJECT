#pragma once
#include "Monster.h"
#include "Client_Defines.h"

enum class BOSSTOMB_SKILL{SPAWN, STUMP, BLUST_END, BLUST_START, BALL, BALL_BREAK, READY_STAR, THROW_STAR, SKIP,SMESH,DEAD,END};

NS_BEGIN(Client)
class CBossTMB final : public CMonster
{
public:
	struct TMB_DESC:public  CMonster::MONSTER_DESC
	{

	};
public:
	DECLARE_DERIVED_TYPE(CBossTMB, CMonster)

private:
	CBossTMB();
	~CBossTMB() override;

public:
	void UpdateGUI() override;
public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void FixedUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;

	void				Set_AttTable(ATTMON eType, _float2 fSkillRatio)override;
	_string				Get_SkillName(ATTMON SkillNode)override;
	const _string		Get_CurSkillName() { return m_CurEffectName; }
	void				Skill_Finished() override;
	_bool				Check_Table(PLAYER_SKILL_TYPE eType)override;

private:

	const _float		Get_Damage() override;
	void				OverLabTest(_vector vSrcPos,  _float fRadius, int32_t iDamage);
	void				Active_Skill();
	void				Active_Dynamic_Effect();
	void				Dead();
private:
	_string			m_EffectNames[ETOUI(BOSSTOMB_SKILL::END)];
	_bool			m_bStar{ true };

	/*----------- 광윤 추가 -----------*/
	std::optional<CHandle> AdditionalLightHandle{};
	/*---------------------------------*/


public:
	static E::UPtr<CBossTMB> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
