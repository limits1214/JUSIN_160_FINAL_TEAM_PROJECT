#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)
class CEdg_Combat : public CState
{
public:
	DECLARE_DERIVED_TYPE(CEdg_Combat, CState)
private:
	CEdg_Combat();
	~CEdg_Combat() override;
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
private:
	void		PhaseCheck();
public:
	static SPtr<CEdg_Combat> Create();
};

NS_END

