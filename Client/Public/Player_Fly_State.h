

#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)

class CPlayer;

class CPlayer_Fly_State final : public CState
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_Fly_State, CState)


	enum class MOVE_DIRECTION : uint32_t
	{
		FRONT,
		RIGHT_45,
		RIGHT_90,
		RIGHT_135,
		BACKWARD,
		LEFT_135,
		LEFT_90,
		LEFT_45,
		END,
	};

	enum class GAIT : uint32_t
	{
		WALK,
		JOG,
		SPRINT,
		END,
	};

	enum class FOOT_PHASE : uint32_t
	{
		LEFT,
		RIGHT,
		END,
	};

	enum class TURN_SIDE : uint32_t
	{
		LEFT,
		RIGHT,
		END,
	};

	enum class TRANSITION : uint32_t
	{
		NONE,
		START,
		STOP,
		GAIT_CHANGE,
		IDLE_TURN,
		PIVOT,
	};

	enum class FLIGHT_PHASE : uint32_t
	{
		MOUNTING,
		HOVER,
		INTO_FLY,
		FLYING,
		INTO_HOVER,
		DISMOUNTING,
	};

private:
	CPlayer_Fly_State();
	~CPlayer_Fly_State() override = default;

public:
	void Enter(CStateMachine* pStateMachine) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Exit(CStateMachine* pStateMachine) override;

	static SPtr<CPlayer_Fly_State> Create();

private:
	void CacheAnimationIndices(const CPlayer& player);
	int32_t ResolveAnimation(const CPlayer& player) const;
	_float CalculateSignedAngle(
		const CPlayer& player,
		const _float3& vMoveDirection) const;
	int32_t FindAnimationIndex(const CPlayer& player, const _string_view& sAnimationName) const;

	int32_t m_iHoverAnimation{ -1 };
	int32_t m_iMountHoverAnimation{ -1 };
	int32_t m_iMountJogAnimation{ -1 };
	int32_t m_iDismountAnimation{ -1 };
	int32_t m_iIntoFlyAnimation{ -1 };
	int32_t m_iIntoHoverAnimation{ -1 };
	int32_t m_iForwardAnimation{ -1 };
	int32_t m_iSlowUpAnimation{ -1 };
	int32_t m_iSlowDownAnimation{ -1 };
	int32_t m_iSlowLeftAnimation{ -1 };
	int32_t m_iSlowRightAnimation{ -1 };
	int32_t m_iFastUpAnimation{ -1 };
	int32_t m_iFastDownAnimation{ -1 };
	int32_t m_iFastLeftAnimation{ -1 };
	int32_t m_iFastRightAnimation{ -1 };
	int32_t m_iActiveAnimation{ -1 };
	_bool m_bAnimationIndicesCached{};
	FLIGHT_PHASE m_eFlightPhase{ FLIGHT_PHASE::MOUNTING };

	_float m_fHoverSpeedThreshold{ 0.25f };
	_float m_fFastSpeedThreshold{ 6.f };
	_float m_fVerticalInputThreshold{ 0.35f };
	_float m_fTurnAngleThreshold{ 55.f };
	_float m_fFacingTurnSpeed{ 180.f };

};

NS_END
