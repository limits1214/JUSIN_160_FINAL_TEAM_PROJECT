#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"
#include "BlackBoardKey.h"
#define EDG_STATE_M  \
X(NONE)\
X(SPAWN)            \
X(COMBAT)            \
X(HIT)               \
X(GROGGY)\
X(PHASE_CHANGE)\
X(DEAD)\
X(END)

NS_BEGIN(Client)
#define X(name) name,
enum class EDG_STATE{ EDG_STATE_M};
#undef X
class CEnderDragon_State final : public CStateMachine
{
public:
	DECLARE_DERIVED_TYPE(CEnderDragon_State, CStateMachine)

private:
	CEnderDragon_State() ;
	CEnderDragon_State(const CEnderDragon_State& rhs);
	~CEnderDragon_State() override ;

private:
	HRESULT		Initialize(void* pArg) override;

public:
	_bool		Add_State(EDG_STATE eState, SPtr<CState> pState);
	_bool		Initialize_State(EDG_STATE eState);
	_bool		Request_State(EDG_STATE eState);
	
	void		ApplyStateRequest();
	void		PriorityUpdate(_float fTimeDelta);
	EDG_STATE	GetCurState() { return m_eCurState; }
private:
	_bool		IsRegistered(EDG_STATE eState);
private:
	std::unordered_set<uint32_t> m_RegisteredState{};
	EDG_STATE					m_eCurState	   { EDG_STATE::NONE};
	EDG_STATE					m_eRequestState{ EDG_STATE::NONE };

public:
	static	UPtr<CEnderDragon_State> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

};

NS_END

