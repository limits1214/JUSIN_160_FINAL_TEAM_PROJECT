#pragma once
#include "Monster.h"
#include "Client_Defines.h"
enum class DRAGON_SKILL{BOOM,BREATH,FIREBALL,SKIP,END};
enum class DRAGON_PHASE{PHASE1, PHASE2, PHASE3, PHASE4, PHASE5, PHASE6, END};
// 투명 드래곤이 울부 짖었다


NS_BEGIN(Client)
class CEnderDragon final : public CMonster
{
public:
	DECLARE_DERIVED_TYPE(CEnderDragon, CMonster)

public:
	typedef struct tagDragonDesc : public CMonster::MONSTER_DESC
	{

	}DRAGON_DESC;

private:
	CEnderDragon();
	~CEnderDragon() override;

public:
	void UpdateGUI() override;
public:
	HRESULT						InitializePrototype(void* pArg = nullptr) override;
	HRESULT						Initialize(void* pArg) override;
	void						PriorityUpdate(E::_float fTimeDelta) override;
	void						FixedUpdate(E::_float fTimeDelta) override;
	void						Update(E::_float fTimeDelta) override;
	void						LateUpdate(E::_float fTimeDelta) override;
	HRESULT						Ready_Fsm(const _string& LevelTag);
	HRESULT						Ready_Skill(const _string& LevelTag);
	void						Ready_BBKeyValue();
public:
	_string						Get_SkillName(ATTMON SkillNode)override;

	void						Set_StateFinished(_bool bFinished);
	void						Set_Break(_bool bHit) { m_bIsBreak = bHit; }

	_bool						Is_StateFinished();
	
	_bool						Check_Table(PLAYER_SKILL_TYPE eType) override;
	void						Check_Phase();
	void						Set_AttTable(ATTMON eType, _float2 fSkillRatio) override;
private:
	void						Update_BBToFsm();
	void						Flag_Check(_float fTimeDelta) override;
	_bool						BreakSkillType(PLAYER_SKILL_TYPE eType);
	void						Phase_Debug();
private:
	class CEnderDragon_State* m_pFsm{ nullptr };

	_string			m_EffectNames[ETOUI(DRAGON_SKILL::END)]{};
	CHandle			m_SkillHandle[ETOUI(DRAGON_SKILL::END)]{};
	DRAGON_SKILL	m_eDragonSkill{};
	DRAGON_PHASE	m_ePhase{};
	_bool			m_bIsBreak{ false }, m_bActiveSKill{ false };

	std::array<_bool, ETOUI(DRAGON_PHASE::END)>	m_bPhaseLock{ false };
public:
	static E::UPtr<CEnderDragon> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
