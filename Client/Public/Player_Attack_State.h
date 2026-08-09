
#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer;

class CPlayer_Attack_State final : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Attack_State, CState)

	enum class ATTACK_DIRECTION : uint8_t
	{
		FWD,
		LFT_45,
		LFT_90,
		LFT_135,
		LFT_180,
		RHT_45,
		RHT_90,
		RHT_135,
		RHT_180,
		END
	};
private:
	CPlayer_Attack_State() = default;
	~CPlayer_Attack_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Exit(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

	static SPtr<CPlayer_Attack_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);
	ATTACK_DIRECTION ResolveAttackDirection(const CPlayer& player) const;
	int32_t GetAttackAnimation(ATTACK_DIRECTION eDirection,_bool bHeavy) const;
	_bool PlayDirectionalAttack(CPlayer& player,_bool bHeavy);

	int32_t FindAnimationIndex(const CPlayer& player,_string_view sAnimationName) const;

private:
	static constexpr size_t FORWARD_LIGHT_ANIMATION_COUNT = 13;
	static constexpr size_t FORWARD_HVY_ANIMATION_COUNT = 10;
	static constexpr _float ATTACK_BLEND_DURATION = 0.12f;
	static constexpr _float COMBO_INPUT_START_RATIO = 0.25f;
	static constexpr _float COMBO_LINK_RATIO = 0.20f;
	static constexpr _float COMBO_INPUT_END_RATIO = 0.75f;
	static constexpr _float MOVE_CANCEL_START_RATIO = 0.35f;
	static constexpr _float MOVE_CANCEL_HARD_START_RATIO = 0.3f;
	static constexpr _float LIGHT_FORWARD_MOVE_START_RATIO = 0.05f;
	static constexpr _float LIGHT_FORWARD_MOVE_END_RATIO = 0.1f;
	static constexpr _float LIGHT_FORWARD_MOVE_SPEED = 1.5f;
	static constexpr _float LIGHT_MAGIC_BULLET_FIRE_RATIO = 0.15f;
	static constexpr _float HEAVY_MAGIC_BULLET_FIRE_RATIO = 0.25f;
	static constexpr size_t ATTACK_DIRECTION_COUNT =static_cast<size_t>(ATTACK_DIRECTION::END);


	std::array<int32_t, FORWARD_LIGHT_ANIMATION_COUNT> m_ForwardLightAnimations{};
	std::array<int32_t, FORWARD_HVY_ANIMATION_COUNT > m_ForwardHvyAnimations{};

	std::array<int32_t, ATTACK_DIRECTION_COUNT> m_DirectionalLightAnimations{};
	std::array<int32_t, ATTACK_DIRECTION_COUNT> m_DirectionalHeavyAnimations{};

	size_t m_iCurrentForwardLightAnimation{};
	size_t m_iCurrentForwardHvyAnimation{};
	uint32_t m_iComboCount{};
	_bool m_bAttackQueued{};
	_bool m_bAnimationIndicesCached{};
	_bool m_bPlayingHeavy{};
	_bool m_bMagicBulletFired{};
	_float m_fPreviousAnimRatio{};

private:
	CGameObject* pTarget = nullptr;
};

NS_END
