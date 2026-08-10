#include "pch.h"
#include "Edg_Spawn.h"
#include "EnderDragon.h"
#include "EnderDragon_State.h"
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
#include "ComCharacterMoveIntent.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
NS_USING(Client)
CEdg_Spawn::CEdg_Spawn()
{
}

CEdg_Spawn::~CEdg_Spawn()
{
}
HRESULT CEdg_Spawn::Initialize()
{
	m_PhasePos.push_back(_float3(30.551f, 226.165f, -65.507f));
	m_PhasePos.push_back(_float3(45.920f, 216.792f, -32.945f));
	m_PhasePos.push_back(_float3(41.621f, 191.567f, -24.108f));
	m_PhasePos.push_back(_float3(5.958f, 180.466f,  -33.356f));
	m_PhasePos.push_back(_float3(-14.889,  185.132f, -38.296f));

	
	
	return S_OK;
}
void CEdg_Spawn::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon)
		return;

	pDragon->Set_StateFinished(false);
	
	m_Anims.push_back(EDG_ANIM_FSM{ .iAnimIndex = 
		pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Cnjrd_Fly_Tucked_Loop_anm.bin"),.fBlend = 0.1f});
	m_Anims.push_back(EDG_ANIM_FSM{ .iAnimIndex = 
		pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Cnjrd_Flap_anm.bin"),.fBlend = 0.5f});
	m_Anims.push_back(EDG_ANIM_FSM{ .iAnimIndex =
		pDragon->Find_AnimIndex("AN_SK_ConjuredDragon_LOD0_Skeleton_Drgn_Cnjrd_Fly_To_Hover_anm.bin"),.fBlend = 0.1f});
	//16.775 227.104 -91.734
	//30.551   226.165   -65.507
	//45.920   216.792 -32.945
	//41.621   191.567  -24.108
	// 
	//5.958     180.466  -33.356
	//-32.252  201.250  -41.125
	
	//스폰용
	//m_iEffectID = CGameInstance::Get().PlayEffect(SkillName, *pDragon->GetTransform().GetWorldMatrix(), _vector{},
	//	[this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
	//	{
	//		if (effectId != m_iEffectID)
	//			return;
	//		m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
	//	});
	
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
	XMStoreFloat3(&vLeftPos, XMLoadFloat3(&vPos)  + -vDir * 15.f);
	XMStoreFloat3(&vRightPos, XMLoadFloat3(&vPos) + vDir * 15.f);

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

	//이거 뺴야지 나중에
	//if (false == pDragon->Is_StateFinished()) return;
	//카메라랑 샤바샤바 하고 전환

	//switch (m_eSpawn)
	//{
	//case EDG_SPAWN_NUMBER::FIRST:
	//	MoveSpawn(pDragon, fTimeDelta);
	//	break;
	//case EDG_SPAWN_NUMBER::SECOND:
	//	Play_Anim(pDragon, fTimeDelta);
	//	break;
	//case EDG_SPAWN_NUMBER::THIRD:
	//	pDragonFsm->Request_State(EDG_STATE::COMBAT);
	//	break;
	//}
	pDragonFsm->Request_State(EDG_STATE::COMBAT);
}

_bool CEdg_Spawn::MoveSpawn(CEnderDragon* pDragon, _float fTimeDelta)
{
	auto pMoveIntent = pDragon->Get_MoveIntent();
	if (nullptr == pMoveIntent)return false;
	
	if (m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
		CGameInstance::Get().SetEffectWorldMatrix(m_iEffectID, *pDragon->GetTransform().GetWorldMatrix());

	if (m_PhasePos.empty())
	{
		m_eSpawn = EDG_SPAWN_NUMBER::SECOND;
		CGameInstance::Get().StopEffect(m_iEffectID);
		return true;
	}

	_vector vNextPos = XMLoadFloat3(&m_PhasePos.front());
	_vector vCurPos = XMLoadFloat3(&pDragon->GetTransform().GetPosition());

	_vector vToNext = vNextPos - vCurPos;
	if (!m_bNext)
	{
		XMStoreFloat3(&m_vLastDir, XMVector3Normalize(pDragon->GetTransform().GetState(STATE::LOOK)));
		XMStoreFloat3(&m_vNextDir, XMVector3Normalize(vNextPos - vCurPos));
		m_bNext = true;
	}
	_float fDot = XMVectorGetX(XMVector3Dot(XMVector3Normalize(vToNext), XMLoadFloat3(&m_vNextDir)));
	_float fDist = XMVectorGetX(XMVector3Length(vNextPos - vCurPos));
	if (fDist <= 0.5f || fDot < 0.f)
	{
		m_PhasePos.pop_front();
		m_bNext = false;
		m_fTick = 0.f;
	}

	m_fTick += fTimeDelta;
	_float t = m_fTick / 1.5f;
	if (t >= 1.f)
		t = 1.f;

	_float3 vLerpDir{};
	XMStoreFloat3(&vLerpDir, XMVector3Normalize(XMVectorLerp(XMLoadFloat3(&m_vLastDir), XMLoadFloat3(&m_vNextDir), t)));

	pMoveIntent->SetMoveIntent(vLerpDir, 25.f);
	pMoveIntent->SetFacingIntent(vLerpDir, 25.f);



	return false;
}
void CEdg_Spawn::Play_Anim(CEnderDragon* pDragon, _float fTimeDelta)
{
	auto pAnimator = pDragon->Get_Animator();
	if (nullptr == pAnimator) return;


	if (!m_Anims.empty())
	{
		if (pAnimator->GetFinish())
			m_Anims.pop_front();
	}
	
	if(m_Anims.empty())
	{
		m_eSpawn = EDG_SPAWN_NUMBER::THIRD;
		return;
	}
	
	EDG_ANIM_FSM EdgAnim = m_Anims.front();

	pAnimator->Play_Anim(EdgAnim.iAnimIndex, false, EdgAnim.fBlend);
	

}
SPtr<CEdg_Spawn> CEdg_Spawn::Create()
{
	auto pInstance = ToSPtr(new CEdg_Spawn{});
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to create CEdg_Spawn");
		return nullptr;
	}

	return pInstance;
}
