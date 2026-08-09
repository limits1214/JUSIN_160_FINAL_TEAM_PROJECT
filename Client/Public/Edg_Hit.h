#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"
#include "EnderDragon.h"
NS_BEGIN(Client)
class CEdg_Hit : public CState
{
public:
	DECLARE_DERIVED_TYPE(CEdg_Hit, CState)
private:
	CEdg_Hit();
	~CEdg_Hit() override;
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;

private:
	void Play_Hit_Anim();
private:
	MON_HIT_INFO			m_eHitInfo{};
public:
	static SPtr<CEdg_Hit> Create();
};

NS_END

