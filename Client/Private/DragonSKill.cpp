#include "pch.h"
#include "DragonSkill.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
NS_USING(Client)
CDragonSkill::CDragonSkill()
{
}

CDragonSkill::CDragonSkill(const CDragonSkill& rhs) : CGameObject(rhs)
{
}

CDragonSkill::~CDragonSkill()
{
}

HRESULT CDragonSkill::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CDragonSkill::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	auto pDesc = static_cast<EDG_SKILL_DESC*>(pArg);

	m_hOwner = pDesc->hOwner;
	m_iBoneIndex = pDesc->iBoneIndex;
	m_pxQueryFilter = pDesc->tQueryFilter;
	return S_OK;
}

void CDragonSkill::PriorityUpdate(E::_float fTimeDelta)
{
	auto pSrc = Get_Owner();
	if (nullptr == pSrc)
		SetPendingDestroy();
}

void CDragonSkill::FixedUpdate(E::_float fTimeDelta)
{
}

void CDragonSkill::Update(E::_float fTimeDelta)
{
}

void CDragonSkill::LateUpdate(E::_float fTimeDelta)
{
}


void CDragonSkill::Cancle()
{
}

void CDragonSkill::Spawn_Skill_Effect(const _string& SkillName)
{
	m_iSkillEffID = CGameInstance::Get().PlayEffect(SkillName, *GetTransform().GetWorldMatrix(), _vector{},
		[this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
		{
			if (effectId != m_iSkillEffID)
				return;
			m_iSkillEffID = INVALID_EFFECT_INSTANCE_ID;
		});
}

void CDragonSkill::Life_Check(_float fTimeDelta)
{
	m_fLife += fTimeDelta;

	if (m_fLife >= m_fMaxLife || m_bHit)
	{
		if (m_iBoneIndex != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().StopEffect(m_iBoneIndex);
		ResetValue();
	}
		
}

const _float4x4* CDragonSkill::Get_BoneMatrix()
{

	auto pSrc = Get_Owner();
	if (nullptr == pSrc)
		return nullptr;
	
	_float4x4 CombineMatrix{};

	XMStoreFloat4x4(&CombineMatrix,
		XMLoadFloat4x4(pSrc->Get_CombineBoneMatrix(m_iBoneIndex)) * XMLoadFloat4x4(pSrc->GetTransform().GetWorldMatrix()));
	return &CombineMatrix;
}

void CDragonSkill::ResetValue()
{
	m_bHit = m_bThrow = m_bActive = false;
	m_vDir = {};
	m_fLife = 0.f;
}

CEnderDragon* CDragonSkill::Get_Owner()
{
	auto pSrc = CGameInstance::Get().GetGameObjectByHandleT<CEnderDragon>(m_hOwner);
	if (nullptr == pSrc)
		return nullptr;

	return pSrc;
}

void CDragonSkill::Set_TargetDir(_vector vSrcPos)
{
	auto pSrc = Get_Owner();
	if (nullptr == pSrc) 
		return;
	
	auto pTarget = pSrc->Get_Target();
	if (nullptr == pTarget)
		return;

	_vector vDestPos = XMLoadFloat3(&pTarget->GetTransform().GetPosition());

	XMStoreFloat3(&m_vTargetDir, XMVector3Normalize(vDestPos - vSrcPos));
}

void CDragonSkill::DebugLine(_float3 vPos)
{
	auto pDbgLineRender = CGameInstance::Get().GetDbgLineRender();

	const auto vPreviousColor = pDbgLineRender->GetColor();
	const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
	pDbgLineRender->SetColor({ 0.f, 1.f, 1.f, 1.f });
	pDbgLineRender->SetDepthTest(true);
	pDbgLineRender->AddSphere(m_fRadius, XMMatrixTranslation(vPos.x, vPos.y, vPos.z));
	pDbgLineRender->SetColor(vPreviousColor);
	pDbgLineRender->SetDepthMode(ePreviousDepthMode);
}
