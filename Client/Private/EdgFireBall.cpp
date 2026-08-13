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
	m_fSpeed = 60.f;
	m_fRadius = 0.5f;
	m_fMaxLife = 3.f;








	return S_OK;
}

void CEdgFireBall::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);

	if (!m_bActive) return;

	if (Life_Check(fTimeDelta)) {
		SetPendingDestroy();
		if(m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().StopEffect(m_iEffectID);
	}
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

void CEdgFireBall::Active(EDG_ACSKT_DESC& SkillTable, _vector vOffsetPos)
{
	_float4x4 matB = Get_BoneMatrix(m_iBoneIndex);
	_float4x4 matOffB = Get_BoneMatrix(m_iOffsetBoneIdex);
	_matrix matBone = XMLoadFloat4x4(&matB);
	_matrix matOffset = XMLoadFloat4x4(&matOffB);
	
	GetTransform().SetPosition(matBone.r[3]);

	if(m_eType == DRAGON_SKILL::FIREBALL || m_eType == DRAGON_SKILL::BLACKBALL)
		Set_TargetDir(matBone.r[3]);
	else if (m_eType == DRAGON_SKILL::THREEBALL)
		XMStoreFloat3(&m_vTargetDir, XMVector3Normalize(matOffset.r[3] - matBone.r[3]));

	_vector vDir = XMLoadFloat3(&m_vTargetDir);
	if (XMVectorGetX(XMVector3LengthSq(vDir)) <= FLT_EPSILON)
		vDir = XMVector3Normalize(matBone.r[2]);
	else
		vDir = XMVector3Normalize(vDir);

	XMStoreFloat3(&m_vTargetDir, vDir);

	_vector vWorldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	if (fabsf(XMVectorGetX(XMVector3Dot(vDir, vWorldUp))) > 0.999f)
		vWorldUp = XMVectorSet(0.f, 0.f, 1.f, 0.f);

	_vector vRight = XMVector3Normalize(XMVector3Cross(vWorldUp, vDir));
	_vector vUp = XMVector3Normalize(XMVector3Cross(vDir, vRight));

	_matrix matRotation = XMMatrixIdentity();
	matRotation.r[0] = XMVectorSetW(vRight, 0.f);
	matRotation.r[1] = XMVectorSetW(vUp, 0.f);
	matRotation.r[2] = XMVectorSetW(vDir, 0.f);

	_float4x4 spawnWorld{};
	XMStoreFloat4x4(&spawnWorld, XMMatrixTranslationFromVector(matBone.r[3]));
	
	if (m_eType == DRAGON_SKILL::FIREBALL) {
		m_iEffectID = CGameInstance::Get().PlayEffect("DragonSpit", spawnWorld);
	}
	else if (m_eType == DRAGON_SKILL::BLACKBALL || m_eType == DRAGON_SKILL::THREEBALL) {
		m_iEffectID = CGameInstance::Get().PlayEffect("DragonProj2", spawnWorld);
	}

	GetTransform().SetQuaternion(XMQuaternionNormalize(XMQuaternionRotationMatrix(matRotation)));
	GetTransform().Update();

	if (m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
		CGameInstance::Get().SetEffectWorldMatrix(m_iEffectID, *GetTransform().GetWorldMatrix());

	m_bActive = true;
	m_bHit = false;
	m_fLife = 0.f;
	//Spawn_Skill_Effect(SkillTable.SkillName);
}

void CEdgFireBall::Cancle()
{
	ResetValue();
}

void CEdgFireBall::MoveBall(_float fTimeDelta)
{
	_vector vPos = GetTransform().GetLoadedPostion();
	_vector vDir = XMVector3Normalize(XMLoadFloat3(&m_vTargetDir));
	_vector vNextPos = vPos + vDir * m_fSpeed * fTimeDelta;

	if (MoveSweep(vNextPos))
	{
		_vector vWorldUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);

		if (fabsf(XMVectorGetX(XMVector3Dot(vDir, vWorldUp))) > 0.999f)
			vWorldUp = XMVectorSet(0.f, 0.f, 1.f, 0.f);

		_vector vRight = XMVector3Normalize(XMVector3Cross(vWorldUp, vDir));
		_vector vUp = XMVector3Normalize(XMVector3Cross(vDir, vRight));

		_matrix matRotation = XMMatrixIdentity();
		matRotation.r[0] = XMVectorSetW(vRight, 0.f);
		matRotation.r[1] = XMVectorSetW(vUp, 0.f);
		matRotation.r[2] = XMVectorSetW(vDir, 0.f);

		_vector vQuaternion = XMQuaternionNormalize(XMQuaternionRotationMatrix(matRotation));

		GetTransform().SetPosition(vNextPos);
		GetTransform().SetQuaternion(vQuaternion);
		GetTransform().Update();

		if (m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().SetEffectWorldMatrix(m_iEffectID, *GetTransform().GetWorldMatrix());
	}


	_float3 vstart{GetTransform().GetPosition().x,GetTransform().GetPosition().y -1.5f,GetTransform().GetPosition().z };
	_float3 vend{GetTransform().GetPosition().x,GetTransform().GetPosition().y +1.5f ,GetTransform().GetPosition().z };

	if (m_eType == DRAGON_SKILL::FIREBALL) {
		CGameInstance::Get().AddTrailPoint("SpitTrail", "SpitTrail", GetHandle(), vstart, vend);

	}
	else if (m_eType == DRAGON_SKILL::BLACKBALL || m_eType == DRAGON_SKILL::THREEBALL) {
		CGameInstance::Get().AddTrailPoint("DragonProj2Trail", "DragonProj2Trail", GetHandle(), vstart, vend);
	}

	//CGameInstance::Get().SetEffectWorldMatrix(m_iEffectID, *m_pComTransform->GetWorldMatrix());

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
