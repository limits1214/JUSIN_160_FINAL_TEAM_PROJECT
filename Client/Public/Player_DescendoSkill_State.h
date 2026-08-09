#pragma once
#include "Player_SkillStateBase.h"


NS_BEGIN(Client)


class CPlayer_DescendoSkill_State final : public CPlayer_SkillStateBase
{

public:
	DECLARE_DERIVED_TYPE(CPlayer_DescendoSkill_State, CPlayer_SkillStateBase)


private:
	CPlayer_DescendoSkill_State() = default;
	~CPlayer_DescendoSkill_State() override = default;

public:
	void Enter(CStateMachine* stateMachine) override;
	void Update(CStateMachine* stateMachine, _float deltaTime) override;
	void Exit(CStateMachine* stateMachine) override;

	static SPtr<CPlayer_DescendoSkill_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);
private:
	enum class PHASE
	{
		CAST,
		ATTACK,
		ATTACK_FAILED,
		PUSH,
		RECOVERY
	};

	_bool	m_bAnimationIndicesCached = false;

	int32_t m_DescendoCast_Animation{};
	int32_t m_DescendoEnd_Animation{};
	int32_t m_AttackFail_Animation{};

	PHASE m_ePhase = PHASE::CAST;

	static constexpr _float CAST_START_RATIO = 0.f;
	static constexpr _float CAST_END_RATIO = 0.2f;
	static constexpr _float MONSTER_PUSH_TIME = 0.f;
	static constexpr _float ATTACK_END_RATIO = 0.2f;
	static constexpr _float RECOVERY_EXIT_RATIO = 0.2f;
	_float	m_fAnimRatio = 0.f;
	uint32_t m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
};


NS_END
