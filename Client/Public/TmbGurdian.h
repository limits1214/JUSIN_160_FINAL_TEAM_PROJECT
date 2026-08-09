#pragma once
#include "Monster.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
enum class TOMB_SKILL { JUMP_START, JUMP_END,SLASH,SMASH,SKIP,HIT_ACCIO, STING, HIT_DESCENDO,END };
class CTmbGurdian final : public CMonster
{
public:
	struct TMBGURDIAN_DESC :public  CMonster::MONSTER_DESC
	{
	};
public:
	DECLARE_DERIVED_TYPE(CTmbGurdian, CMonster)

private:
	CTmbGurdian();
	~CTmbGurdian() override;

public:
	void UpdateGUI() override;
private:
	_bool UpdateDeadDebrisPoseFromCurrentBones();
	_bool ActivateDeadDebrisPhysics();
public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void FixedUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;

	virtual void				Set_AttTable(ATTMON eType, _float2 fSkillRatio)override;
	_string						Get_SkillName(ATTMON SkillNode)override;
	const _float				Get_Damage() override;
	_bool						Check_Table(PLAYER_SKILL_TYPE eType) override;
	
private:
	void						Damaged(PLAYER_SKILL_TYPE eType) override;
	void						Active_Skill();
	void						ReadySound();
private:
	std::vector<CHandle> m_vecDeadHandles{};
	std::vector<int32_t> m_vecDeadBoneIndices{};
	std::vector<_float4x4> m_vecDeadInverseBindMatrices{};
	_bool m_bRenderDeadDebris{};
	_bool m_bDeadDebrisPhysicsActivated{};
	
	_string			m_EffectNames[ETOUI(TOMB_SKILL::END)];
	TOMB_SKILL					m_eTombSkill{};
public:
	static E::UPtr<CTmbGurdian> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
