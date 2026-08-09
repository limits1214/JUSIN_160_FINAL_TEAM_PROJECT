#include "pch.h"
#include "Edg_Phase.h"
#include "BTBlackBoard.h"
#include "BlackBoardKey.h"
#include "EnderDragon_State.h"
#include "ComCharacterMoveIntent.h"
NS_USING(Client)
CEdg_Phase::CEdg_Phase()
{
}

CEdg_Phase::~CEdg_Phase()
{
}
HRESULT CEdg_Phase::Initialize()
{

	m_PhasePos[ETOUI(DRAGON_PHASE::PHASE2)].push_back(_float3(20, 0, 20));
	m_PhasePos[ETOUI(DRAGON_PHASE::PHASE2)].push_back(_float3(40, 0, 40));
	m_PhasePos[ETOUI(DRAGON_PHASE::PHASE2)].push_back(_float3(80, 0, 80));
	m_PhasePos[ETOUI(DRAGON_PHASE::PHASE2)].push_back(_float3(40, 0, 40));
	m_PhasePos[ETOUI(DRAGON_PHASE::PHASE2)].push_back(_float3(20, 0, 20));

	return S_OK;
}
void CEdg_Phase::Enter(CStateMachine* pStateMachine)
{
	CEnderDragon* pDragon = pStateMachine->GetOwner<CEnderDragon>();

	if (nullptr == pDragon) return;

	auto pBB = pDragon->Get_BlackBoard();
	if (nullptr == pBB) return;

	auto pPhase = pBB->Get_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE);
	if (nullptr == pPhase) return;

	m_ePhase = *pPhase;
	m_eNextPhase = DRAGON_PHASE::END;
	m_bNext = false;

}

void CEdg_Phase::Exit(CStateMachine* pStateMachine)
{
	auto pDragon = pStateMachine->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	pDragon->ReActiveTable();
	pDragon->Set_StateFinished(false);
}

void CEdg_Phase::PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta)
{
	
}

void CEdg_Phase::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto pDragonFsm = Cast<CEnderDragon_State>(pStateMachine);
	if (nullptr == pDragonFsm) return;

	auto pDragon = pDragonFsm->GetOwner<CEnderDragon>();
	if (nullptr == pDragon) return;

	Phase_Change_Action(pDragon, fTimeDelta);

	//도망치는게 끝나면 다시 상태전환 하기
	if (m_eNextPhase != DRAGON_PHASE::END)
	{
		auto pBB = pDragon->Get_BlackBoard();
		if (nullptr == pBB) return;

		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, m_eNextPhase);
		pDragonFsm->Request_State(EDG_STATE::COMBAT);
	}
}
_bool CEdg_Phase::MovePhase(CEnderDragon* pDragon, _float fTimeDelta)
{
	auto pMoveIntent = pDragon->Get_MoveIntent();
	if (nullptr == pMoveIntent)return false;
	
	if (m_PhasePos[ETOUI(m_ePhase)].empty()) return true;
	
	_vector vNextPos = XMLoadFloat3(&m_PhasePos[ETOUI(m_ePhase)].front());
	_vector vCurPos  = XMLoadFloat3(&pDragon->GetTransform().GetPosition());

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
		m_PhasePos[ETOUI(m_ePhase)].pop_front();
		m_bNext = false;
		m_fTick = 0.f;
	}

	m_fTick += fTimeDelta;
	_float t = m_fTick / 3.f;
	if (t >= 1.f)
		t = 1.f;

	_float3 vLerpDir{};
	XMStoreFloat3(&vLerpDir, XMVector3Normalize(XMVectorLerp(XMLoadFloat3(&m_vLastDir), XMLoadFloat3(&m_vNextDir), t)));
	
	pMoveIntent->SetMoveIntent(vLerpDir, 5.f);
	pMoveIntent->SetFacingIntent(vLerpDir, 6.f);
	return false;
}
void CEdg_Phase::Phase_Change_Action(CEnderDragon* pDragon, _float fTimeDelta)
{
	if (MovePhase(pDragon, fTimeDelta))
		m_eNextPhase = m_ePhase;
}
SPtr<CEdg_Phase> CEdg_Phase::Create()
{
	auto pInstance = ToSPtr(new CEdg_Phase{});
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to create CEdg_Phase");
		return nullptr;
	}

	return pInstance;
}
