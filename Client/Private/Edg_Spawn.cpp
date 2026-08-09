#include "pch.h"
#include "Edg_Spawn.h"
#include "EnderDragon.h"
#include "EnderDragon_State.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
NS_USING(Client)
CEdg_Spawn::CEdg_Spawn()
{
}

CEdg_Spawn::~CEdg_Spawn()
{
}

void CEdg_Spawn::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon)
		return;

	pDragon->Set_StateFinished(false);
}

void CEdg_Spawn::Exit(CStateMachine* pStateMachine)
{
	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;
	
	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;

	_float3 vLeftPos{}, vRightPos{};
	//좌우 무빙
	_float3 vPos = pDragon->GetTransform().GetPosition();
	_vector vDir = XMVector3Normalize(pDragon->GetTransform().GetState(STATE::RIGHT));
	XMStoreFloat3(&vLeftPos, XMLoadFloat3(&vPos)  + -vDir * 25.f);
	XMStoreFloat3(&vRightPos, XMLoadFloat3(&vPos) + vDir * 25.f);

	pBB->Set_Value<_float3>(EDG_KEY::LPATROL, vLeftPos);
	pBB->Set_Value<_float3>(EDG_KEY::RPATROL, vRightPos);
}

void CEdg_Spawn::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
}

void CEdg_Spawn::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pDragonFsm = Cast<CEnderDragon_State>(pStateMachine);
	if (nullptr == pDragonFsm) return;

	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;
	
	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;

	if (false == pDragon->Is_StateFinished()) return;
	//카메라랑 샤바샤바 하고 전환

	pDragonFsm->Request_State(EDG_STATE::COMBAT);
}
SPtr<CEdg_Spawn> CEdg_Spawn::Create()
{
	auto pInstance = ToSPtr(new CEdg_Spawn{});
	if (!pInstance)
	{
		MSG_BOX("Failed to create CEdg_Spawn");
		return nullptr;
	}

	return pInstance;
}
