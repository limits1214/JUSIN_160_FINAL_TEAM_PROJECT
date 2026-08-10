#include "pch.h"
#include "EdgFireBall.h"
#include "PhysXManager.h"
NS_USING(Client)
CEdgFireBall::CEdgFireBall()
{
}

CEdgFireBall::CEdgFireBall(const CEdgFireBall& rhs) : CDragonSkill(rhs)
{
}

CEdgFireBall::~CEdgFireBall()
{
}

HRESULT CEdgFireBall::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CEdgFireBall::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
	{
		MSG_BOX("Create Failed EDG FireBall");
		return E_FAIL;
	}
	m_fDamage = 30.f;
	m_fSpeed = 100.f;
	m_fRadius = 0.5f;
	m_fMaxLife = 3.f;
	return S_OK;
}

void CEdgFireBall::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);

	if (!m_bActive) return;

	if (Life_Check(fTimeDelta))
		SetPendingDestroy();
}

void CEdgFireBall::FixedUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::FixedUpdate(fTimeDelta);
	MoveBall(fTimeDelta);
}

void CEdgFireBall::Update(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::Update(fTimeDelta);
}

void CEdgFireBall::LateUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::LateUpdate(fTimeDelta);
}

void CEdgFireBall::Active(const _string& SkillName)
{
	_float4x4 matB = Get_BoneMatrix(m_iBoneIndex);
	_float4x4 matOffB = Get_BoneMatrix(m_iOffsetBoneIdex);
	_matrix matBone = XMLoadFloat4x4(&matB);
	_matrix matOffset = XMLoadFloat4x4(&matOffB);

	_vector vQuat = XMQuaternionRotationMatrix(matBone);
	
	GetTransform().SetPosition(matBone.r[3]);
	GetTransform().SetQuaternion(vQuat);
	GetTransform().Update();
	//Set_TargetDir(matBone.r[3]);
	XMStoreFloat3(&m_vTargetDir, XMVector3Normalize(matOffset.r[3] - matBone.r[3]));
	m_bActive = true;
	m_bHit = false;
	m_fLife = 0.f;
	Spawn_Skill_Effect(SkillName);
}

void CEdgFireBall::Cancle()
{
	ResetValue();
}

void CEdgFireBall::MoveBall(_float fTimeDelta)
{
	_vector vPos = GetTransform().GetLoadedPostion();

	_vector vDir = XMLoadFloat3(&m_vTargetDir);

	_vector vNextPos = vPos + vDir * m_fSpeed * fTimeDelta;

	if (MoveSweep(vNextPos))
	{
		if (m_iBoneIndex != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().SetEffectWorldMatrix(m_iBoneIndex, *GetTransform().GetWorldMatrix());
		GetTransform().SetPosition(vNextPos);
		GetTransform().Update();
	}

}

_bool CEdgFireBall::MoveSweep(_vector vNextPos)
{
	_float3 vPos = GetTransform().GetPosition();

	_vector vDisPlacement = vNextPos - XMLoadFloat3(&vPos);

	_float fDist = XMVectorGetX(XMVector3Length(vDisPlacement));

	_float3 vDir = {};
	XMStoreFloat3(&vDir, XMVector3Normalize(vDisPlacement));

	PX_SWEEP_DESC SweepDesc{};
	SweepDesc.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	SweepDesc.tGeometry.fRadius = m_fRadius;
	SweepDesc.tPose.vPosition = vPos;
	SweepDesc.vDirection = vDir;
	SweepDesc.tFilter = m_pxQueryFilter;
	SweepDesc.fMaxDistance = fDist;

	////////////////////////////////////
	DebugLine(SweepDesc.tPose.vPosition);
	/////////////////////////////////////

	PX_SWEEP_RESULT SweepResult{} ;
	auto pPhysX = CGameInstance::Get().GetPhysXManager();
	if (nullptr == pPhysX) return false;

	if (pPhysX->Sweep(SweepDesc, SweepResult) && SweepResult.bHit)
	{
		m_bHit = true;
		//펑
		return false;
	}
	return true;
}

E::UPtr<CEdgFireBall> CEdgFireBall::Create()
{
	auto pInstance = E::ToUPtr(new CEdgFireBall{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CEdgFireBall");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CEdgFireBall::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CEdgFireBall{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CEdgFireBall");
		return nullptr;
	}

	return pInstance;
}
