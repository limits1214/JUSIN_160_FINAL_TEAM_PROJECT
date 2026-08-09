#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)
class CEdg_Spawn : public CState
{
public:
	DECLARE_DERIVED_TYPE(CEdg_Spawn, CState)
private:
	CEdg_Spawn();
	~CEdg_Spawn() override;
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
public:
	static SPtr<CEdg_Spawn> Create();
};

NS_END

