
#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)
enum class PLAYER_STATE : uint32_t
{
	NONE = 0,
	LOCOMOTION,
	ROLL,
	JUMP,
	ATTACK,
	SKILL_BEGIN,
	DASH_SKILL,
	ACIENTATTACK_SKILL,
	ACCIO_SKILL,
	DEPULSO_SKILL,
	DESCENDO_SKILL,
	REVELIO_SKILL,
	REPAIRO_SKILL,
	SKILL_END,
	HIT,
	DEAD,
	FLY,
	END,
};

//FSM 훔쳐버리기
class CPlayer_StateMachine final : public CStateMachine
{
public:
	DECLARE_DERIVED_TYPE(CPlayer_StateMachine, CStateMachine)

private:
	CPlayer_StateMachine() = default;
	CPlayer_StateMachine(const CPlayer_StateMachine& rhs);
	~CPlayer_StateMachine() override = default;

private:
	HRESULT Initialize(void* pArg) override;

public:
	_bool AddPlayerState(PLAYER_STATE eState, SPtr<CState> pState);
	_bool SetInitialState(PLAYER_STATE eState);

	_bool RequestState(PLAYER_STATE eState);
	void ApplyStateRequest();

	void PriorityUpdate(_float fTimeDelta);

	PLAYER_STATE GetCurrentState() const { return m_eCurrentState; }
	PLAYER_STATE GetRequestedState() const { return m_eRequestedState; }
	_bool IsInSkillState() const;

	static UPtr<CPlayer_StateMachine> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	_bool IsRegistered(PLAYER_STATE eState) const;
	static _bool IsSkillState(PLAYER_STATE eState);
	_bool CanTransition(PLAYER_STATE eCurrent, PLAYER_STATE eNext) const;
	uint32_t GetTransitionPriority(PLAYER_STATE eState) const;

private:
	std::unordered_set<uint32_t> m_RegisteredStateIDs{};
	PLAYER_STATE m_eCurrentState = PLAYER_STATE::NONE;
	PLAYER_STATE m_eRequestedState = PLAYER_STATE::NONE;

private:
	void Free() override;
};

NS_END
