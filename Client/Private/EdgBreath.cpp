#include "pch.h"
#include "EdgBreath.h"
#include "PhysXManager.h"
NS_USING(Client)
CEdgBreath::CEdgBreath()
{
}

CEdgBreath::CEdgBreath(const CEdgBreath& rhs) : CDragonSkill(rhs)
{
}

CEdgBreath::~CEdgBreath()
{
}

HRESULT CEdgBreath::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CEdgBreath::Initialize(void* pArg)
{
	auto pDesc = static_cast<EDG_SKILL_DESC*>(pArg);
	pDesc->tQueryFilter.bQueryStatic = false;
	if (FAILED(__super::Initialize(pArg)))
	{
		MSG_BOX("Create Failed EDG Breath");
		return E_FAIL;
	}
	m_fDamage = 3.f;
	m_fRadius = 1.2f;
	m_fMaxLife = 2.f;
	return S_OK;
}

void CEdgBreath::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);

	if (!m_bActive) return;

	Life_Check(fTimeDelta);
}

void CEdgBreath::FixedUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::FixedUpdate(fTimeDelta);
	MoveBreath(fTimeDelta);
}

void CEdgBreath::Update(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::Update(fTimeDelta);
}

void CEdgBreath::LateUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::LateUpdate(fTimeDelta);
}

void CEdgBreath::Active(const _string& SkillName)
{
	auto pSrc = Get_Owner();
	if (nullptr == pSrc) return;
	auto pDest = pSrc->Get_Target();
	if (nullptr == pDest) return;

	 _float4x4 BoneMatrix = Get_BoneMatrix(m_iBoneIndex);
	_matrix matBone = XMLoadFloat4x4(&BoneMatrix); 
	_vector vDestPos = XMLoadFloat3(&pDest->GetTransform().GetPosition());


	_vector vLength = vDestPos - matBone.r[3];
	_vector vDir = XMVector3Normalize(vLength);
	_float fHalfDis = XMVectorGetX(XMVector3Length(vLength)) * 0.5f;
	
	
	XMStoreFloat3(&m_vTargetDir, vDir);
	XMStoreFloat3(&m_vDir, XMVector3Normalize((matBone.r[3] + vDir * fHalfDis) - matBone.r[3]));

	m_bActive = true;
	m_bHit = false;
	m_fBreathTick = m_fLife = 0.f;
	Spawn_Skill_Effect(SkillName);
}

void CEdgBreath::Cancle()
{
	ResetValue();
}

void CEdgBreath::MoveBreath(_float fTimeDelta)
{
	 _float4x4 BoneMatrix = Get_BoneMatrix(m_iBoneIndex);
	_matrix matBone = XMLoadFloat4x4(&BoneMatrix);
	_vector vQuat = XMQuaternionRotationMatrix(matBone);

	m_fBreathTick += fTimeDelta;

	_float t = std::min(m_fBreathTick / 3.f, 1.f);
	_vector vForward = XMVector3Normalize(XMVectorLerp(XMLoadFloat3(&m_vDir), XMLoadFloat3(&m_vTargetDir), t));
	_vector vUp = XMVector3Normalize(matBone.r[1]);

	if (fabsf(XMVectorGetX(XMVector3Dot(vForward, vUp))) > 0.98f)
		vUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	_vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vForward));
	vUp = XMVector3Normalize(XMVector3Cross(vForward, vRight));

	_matrix breathWorld = XMMatrixIdentity();
	breathWorld.r[0] = XMVectorSetW(vRight, 0.f);
	breathWorld.r[1] = XMVectorSetW(vUp, 0.f);
	breathWorld.r[2] = XMVectorSetW(vForward, 0.f);
	breathWorld.r[3] = XMVectorSetW(matBone.r[3], 1.f);

	if (MoveSweep(matBone.r[3], vForward))
	{
		if (m_iSkillEffID != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().SetEffectWorldMatrix(m_iSkillEffID, *GetTransform().GetWorldMatrix());
		GetTransform().SetPosition(matBone.r[3]);
		GetTransform().SetQuaternion(vQuat);
		GetTransform().Update();
	
	}
	
}

_bool CEdgBreath::MoveSweep(_vector vNextPos, _vector vCurDir)
{
	_float3 vPos{}, vDir{};
	
	XMStoreFloat3(&vPos, vNextPos);
	XMStoreFloat3(&vDir, vCurDir);

	uint32_t iDebugCnt = 20;

	PX_SWEEP_DESC SweepDesc{};
	SweepDesc.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	SweepDesc.tGeometry.fRadius = m_fRadius;
	SweepDesc.tPose.vPosition = vPos;
	SweepDesc.vDirection = vDir;
	SweepDesc.tFilter = m_pxQueryFilter;
	SweepDesc.fMaxDistance = 60.f;

	////////////////////////////////////
	for (uint32_t i = 0; i < iDebugCnt; ++i)
	{
		_float t = static_cast<_float>(i) / static_cast<_float>(iDebugCnt);
		_float3 vDebugPos{};
		XMStoreFloat3(&vDebugPos, XMLoadFloat3(&SweepDesc.tPose.vPosition) + XMLoadFloat3(&vDir) * SweepDesc.fMaxDistance * t);
		DebugLine(vDebugPos);
	}
	/////////////////////////////////////

	PX_SWEEP_RESULT SweepResult{};
	auto pPhysX = CGameInstance::Get().GetPhysXManager();
	if (nullptr == pPhysX) return false;

	if (pPhysX->Sweep(SweepDesc, SweepResult) && SweepResult.bHit)
	{
		return false;
	}

	return true;
}

E::UPtr<CEdgBreath> CEdgBreath::Create()
{
	auto pInstance = E::ToUPtr(new CEdgBreath{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CEdgBreath");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CEdgBreath::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CEdgBreath{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CEdgBreath");
		return nullptr;
	}

	return pInstance;
}
