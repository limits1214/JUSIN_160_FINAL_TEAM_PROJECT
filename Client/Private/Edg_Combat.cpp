#include "pch.h"
#include "Edg_Combat.h"
#include "EnderDragon.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
NS_USING(Client)
CEdg_Combat::CEdg_Combat()
{
}

CEdg_Combat::~CEdg_Combat()
{
}

void CEdg_Combat::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon)
		return;

}

void CEdg_Combat::Exit(CStateMachine* pStateMachine)
{
}

void CEdg_Combat::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CEdg_Combat::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	auto pBB = pDragon->Get_BlackBoard();
	if (pBB == nullptr) return;

	
}
SPtr<CEdg_Combat> CEdg_Combat::Create()
{
	auto pInstance = ToSPtr(new CEdg_Combat{});
	if (!pInstance)
	{
		MSG_BOX("Failed to create CEdg_Combat");
		return nullptr;
	}

	return pInstance;
}
