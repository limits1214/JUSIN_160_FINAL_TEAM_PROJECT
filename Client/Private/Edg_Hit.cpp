#include "pch.h"
#include "Edg_Hit.h"
#include "BTBlackBoard.h"
#include "BlackBoardKey.h"
#include "EnderDragon_State.h"
NS_USING(Client)
CEdg_Hit::CEdg_Hit()
{
}

CEdg_Hit::~CEdg_Hit()
{
}

void CEdg_Hit::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon) return;
	if (false == pDragon->Activate_PendingHit()) return;

	m_eHitInfo = pDragon->Get_ActiveHitInfo();
}

void CEdg_Hit::Exit(CStateMachine* pStateMachine)
{
	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	pDragon->Set_Break(false);
	pDragon->ReActiveTable();
}

void CEdg_Hit::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CEdg_Hit::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pDragonFsm = Cast<CEnderDragon_State>(pStateMachine);
	if (nullptr == pDragonFsm) return;

	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	auto pAnimator = pDragon->Get_Animator();
	if (nullptr == pAnimator) return;

	Play_Hit_Anim();

}

void CEdg_Hit::Play_Hit_Anim()
{
	//윽
}

SPtr<CEdg_Hit> CEdg_Hit::Create()
{
	auto pInstance = ToSPtr(new CEdg_Hit{});
	if (!pInstance)
	{
		MSG_BOX("Failed to create CEdg_Hit");
		return nullptr;
	}

	return pInstance;
}
