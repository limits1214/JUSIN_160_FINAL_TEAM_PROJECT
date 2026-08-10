#include "pch.h"
#include "EdgPulse.h"
#include "PhysXManager.h"
#include "Player.h"
NS_USING(Client)
CEdgPulse::CEdgPulse()
{
}

CEdgPulse::CEdgPulse(const CEdgPulse& rhs) : CDragonSkill(rhs)
{
}

CEdgPulse::~CEdgPulse()
{
}

HRESULT CEdgPulse::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CEdgPulse::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
	{
		MSG_BOX("Create Failed EDG CEdgPulse");
		return E_FAIL;
	}
	m_fDamage = 70.f;
	m_fSpeed = 20.f;
	m_fRadius = 1.5f;
	m_fMaxLife = 3.f;
	return S_OK;
}

void CEdgPulse::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);

	if (!m_bActive) return;

	Life_Check(fTimeDelta);
}

void CEdgPulse::FixedUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::FixedUpdate(fTimeDelta);
	Pulse(fTimeDelta);
}

void CEdgPulse::Update(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::Update(fTimeDelta);
}

void CEdgPulse::LateUpdate(E::_float fTimeDelta)
{
	if (!m_bActive) return;

	__super::LateUpdate(fTimeDelta);
}

void CEdgPulse::Active(const _string& SkillName)
{
	_float4x4 BoneMatrix = Get_BoneMatrix(m_iBoneIndex);

	_matrix matBone = XMLoadFloat4x4(&BoneMatrix);
	_vector vQuat = XMQuaternionRotationMatrix(matBone);

	GetTransform().SetPosition(matBone.r[3]);
	GetTransform().SetQuaternion(vQuat);
	GetTransform().Update();
	m_bActive = true;
	m_bHit = false;
	m_fLife = 0.f;
	m_fRadius = 1.5f;
	Spawn_Skill_Effect(SkillName);
}

void CEdgPulse::Cancle()
{
	ResetValue();
}

void CEdgPulse::Pulse(_float fTimeDelta)
{
	 _float4x4 BoneMatrix = Get_BoneMatrix(m_iBoneIndex);
	_matrix matBone = XMLoadFloat4x4(&BoneMatrix);
	_vector vQuat = XMQuaternionRotationMatrix(matBone);
	m_fRadius += m_fSpeed * fTimeDelta;
	if (PulseSweep(matBone.r[3]))
	{
		if (m_iBoneIndex != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().SetEffectWorldMatrix(m_iBoneIndex, *GetTransform().GetWorldMatrix());
		GetTransform().SetPosition(matBone.r[3]);
		GetTransform().Update();
	}

}

_bool CEdgPulse::PulseSweep(_vector vNextPos)
{
	_float3 vPos = {};
	XMStoreFloat3(&vPos, vNextPos);

	PX_SWEEP_DESC SweepDesc{};
	SweepDesc.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	SweepDesc.tGeometry.fRadius = m_fRadius;
	SweepDesc.tPose.vPosition = vPos;
	SweepDesc.tFilter = m_pxQueryFilter;

	////////////////////////////////////
	DebugLine(SweepDesc.tPose.vPosition);
	/////////////////////////////////////

	PX_SWEEP_RESULT SweepResult{};
	auto pPhysX = CGameInstance::Get().GetPhysXManager();
	if (nullptr == pPhysX) return false;

	if (pPhysX->Sweep(SweepDesc, SweepResult) && SweepResult.bHit)
	{
		//auto pTarget = CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(SweepResult.hGameObject);
		//pTarget->OnQueryHit(m_fDamage);

		m_bHit = true;
		return false;
	}
	return true;
}

E::UPtr<CEdgPulse> CEdgPulse::Create()
{
	auto pInstance = E::ToUPtr(new CEdgPulse{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CEdgPulse");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CEdgPulse::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CEdgPulse{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CEdgPulse");
		return nullptr;
	}

	return pInstance;
}
