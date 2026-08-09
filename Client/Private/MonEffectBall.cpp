#include "pch.h"
#include "MonEffectBall.h"
#include "Client_Resources.h"
#include "Trail_CPU.h"
#include "PhysXManager.h"
#include "BossTMB.h"
#include "ComModelInstance.h"
#include "Player.h"
#include "ComBeHavior.h"
NS_USING(Client)

CMonEffectBall::CMonEffectBall()
	: CGameObject{}
{
}

CMonEffectBall::~CMonEffectBall()
{
}

void CMonEffectBall::UpdateGUI()
{
	CGameObject::UpdateGUI();

}

HRESULT CMonEffectBall::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CMonEffectBall::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	//뎀지
	m_iDamage = 30.f;
	const auto pDesc = static_cast<const MON_BALL*>(pArg);
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	m_hParent = pDesc->hOwner;
	m_hTarget = pDesc->hTarget;
	m_iBoneIndex = pDesc->iBoneIndex;
	m_tQueryFilter = pDesc->tQueryFilter;
	m_iDamage = pDesc->fDamage;
	GetTransform().Update();
	m_iEffectID = CGameInstance::Get().PlayEffect("BossRingAttack", *GetTransform().GetWorldMatrix(), _vector{},
		[h = GetHandle()](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
		{
			if (auto pOBJ = CGameInstance::Get().GetGameObjectByHandleT<CMonEffectBall>(h)) {
				pOBJ->m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
			}
		});
	m_fDeadTime = 0.f;
	XMStoreFloat4x4(&m_Offsetmat, XMMatrixIdentity());
	return S_OK;
}

void CMonEffectBall::PriorityUpdate(E::_float fTimeDelta)
{
}

void CMonEffectBall::FixedUpdate(E::_float fTimeDelta)
{
}

void CMonEffectBall::Update(E::_float fTimeDelta)
{
	if (!m_bHit)
	OverlapTest();

	if (m_bHit || m_iEffectID == INVALID_EFFECT_INSTANCE_ID || m_fDeadTime > 1.f) {

		_float4x4 mat = *GetTransform().GetWorldMatrix();
		const _matrix effectWorld =
			XMMatrixTranslation(
				m_CurWorldmat._41,
				m_CurWorldmat._42,
				m_CurWorldmat._43);
		_float4x4 effectMat;
		XMStoreFloat4x4(&effectMat, effectWorld);
		if (m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
		{
			CGameInstance::Get().StopEffect(m_iEffectID);
			m_iEffectID = INVALID_EFFECT_INSTANCE_ID;
		}
		if (!m_bPatternBroken)
		{
			CGameInstance::Get().PlayEffect(
				"BossRingAttackAfterEffect",
				effectMat,
				_vector{});
		}

		SetPendingDestroy();
		return;
	}

}

void CMonEffectBall::LateUpdate(E::_float fTimeDelta)
{
	Chase(fTimeDelta);

	auto cachedCol = CGameInstance::Get().GetDbgLineRender()->GetColor();
	auto cachedDepth = CGameInstance::Get().GetDbgLineRender()->GetDepthMode();
	CGameInstance::Get().GetDbgLineRender()->SetColor({ 1.f, 0.f, 0.f, 1.f });
	CGameInstance::Get().GetDbgLineRender()->SetDepthTest(false);
	CGameInstance::Get().GetDbgLineRender()->AddSphere(5.f, XMLoadFloat4x4(&m_CurWorldmat));
	CGameInstance::Get().GetDbgLineRender()->SetColor(cachedCol);
	CGameInstance::Get().GetDbgLineRender()->SetDepthMode(cachedDepth);
}

HRESULT CMonEffectBall::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

void CMonEffectBall::OverlapTest()
{
	if (!m_bThrow)
		return;
	_float3 vPos{
			m_CurWorldmat._41,
			m_CurWorldmat._42,
			m_CurWorldmat._43
	};

	PX_OVERLAP_DESC desc{};
	desc.tGeometry = {
		.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE,
		.fRadius = 1.2f
	};
	desc.tPose = { .vPosition = vPos };

	// 플레이어 검사
	desc.tFilter = {
		.iQueryMask = ETOUI(COLLISION_LAYER::PLAYER_HURTBOX),
		.bQueryStatic = false,
		.bQueryDynamic = true
	};

	PX_OVERLAP_RESULT result{};
	if (CGameInstance::Get().GetPhysXManager()->Overlap(desc, result))
	{
		if (auto pTarget =
			CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(
				result.hGameObject))
		{
			pTarget->OnQueryHit(m_iDamage);
			m_bHit = true;
			return;
		}
	}

	// 바닥/벽 검사
	desc.tFilter = {
		.iQueryMask = ETOUI(COLLISION_LAYER::WORLD_STATIC),
		.bQueryStatic = true,
		.bQueryDynamic = false
	};

	result = {};
	if (CGameInstance::Get().GetPhysXManager()->Overlap(desc, result))
	{
		m_bHit = true; // 다음 종료 처리에서 폭발 이펙트
		return;
	}
}

void CMonEffectBall::Chase(_float fTimeDelta)
{
	if (m_bHit) return;
	auto Owner = CGameInstance::Get().GetGameObjectByHandle(m_hParent);
	if (!Owner) return;

	auto pModel = Owner->GetComponent<CComModelInstance>("ComCModelIntance");
	if (!pModel) return;

	auto pBT = Owner->GetComponent<CComBeHavior>("Com_BT");
	if (!pBT) return;

	_bool bThrow = pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::THROW));

	if (!m_bThrow)
	{
		const auto& boneMatrices = pModel->Get_CombinedBoneMatrices();
		if (m_iBoneIndex >= boneMatrices.size())
			return;

		_matrix matBone = XMLoadFloat4x4(&boneMatrices[m_iBoneIndex]);

		for (uint32_t i = 0; i < 3; ++i)
			matBone.r[i] = XMVector3Normalize(matBone.r[i]);

		_matrix matWorld = matBone * XMLoadFloat4x4(pModel->GetGameObject()->GetTransform().GetWorldMatrix());
		XMStoreFloat4x4(&m_CurWorldmat, matWorld);

		if (bThrow)
		{
			auto pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hTarget);
			if (!pTarget) return;

			_vector vBallPos = XMLoadFloat4x4(&m_CurWorldmat).r[3];
			_vector vTargetPos = XMLoadFloat3(&pTarget->GetTransform().GetPosition());

			_vector vTargetDir = vTargetPos - vBallPos;

			XMStoreFloat3(&m_vDir, XMVector3Normalize(vTargetDir));
			m_bThrow = true;
			m_fDeadTime = 0.f;
		}
	}
	if (m_bThrow)
	{
		m_fDeadTime += fTimeDelta;

		_matrix matBall = XMLoadFloat4x4(&m_CurWorldmat);

		_vector vPos = matBall.r[3];
		_vector vDir = XMLoadFloat3(&m_vDir);

		vPos += vDir * m_fSpeed * fTimeDelta;

		matBall.r[3] = XMVectorSetW(vPos, 1.f);
		XMStoreFloat4x4(&m_CurWorldmat, matBall);
	}

	if (pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::NOCKDOWN) | ETOUI(CBTRoot::BTFLAG::GROGY) | ETOUI(CBTRoot::BTFLAG::DEAD)))
	{
		CGameInstance::Get().StopEffect(m_iEffectID);
		m_bHit = true;

		m_bPatternBroken = true;
		auto pCamera = CGameInstance::Get().GetActiveCamera();
		pBT->Set_Flag(ETOUI(CBTRoot::BTFLAG::NOCKDOWN), FLAGTYPE::DEL);
		if (!pCamera)
			return;
		_matrix currentWorld = XMLoadFloat4x4(&m_CurWorldmat);
		_vector effectPosition = currentWorld.r[3];

		_matrix cameraWorld = XMMatrixInverse(nullptr, pCamera->GetView());

		_vector cameraRight = XMVector3Normalize(cameraWorld.r[0]);
		_vector cameraUp = XMVector3Normalize(cameraWorld.r[1]);
		_vector cameraLook = XMVector3Normalize(cameraWorld.r[2]);

		_matrix effectMatrix = XMMatrixIdentity();
		effectMatrix.r[0] = XMVectorSetW(cameraRight, 0.f);
		effectMatrix.r[1] = XMVectorSetW(cameraUp, 0.f);
		effectMatrix.r[2] = XMVectorSetW(cameraLook, 0.f);
		effectMatrix.r[3] = XMVectorSetW(effectPosition, 1.f);

		_float4x4 effectWorld{};
		XMStoreFloat4x4(&effectWorld, effectMatrix);

		CGameInstance::Get().Set_ChromaticRingOpacity(0.2f);
		CGameInstance::Get().Render_ChromaticRing(effectPosition, 0.5f, 100);
		CGameInstance::Get().PlayEffect(
			"TombBossPatternBlockEffect",
			effectWorld,
			_vector{});

		return;
	}

	if (m_iEffectID != INVALID_EFFECT_INSTANCE_ID)
		CGameInstance::Get().SetEffectWorldMatrix(m_iEffectID, m_CurWorldmat);

}

E::UPtr<CMonEffectBall> CMonEffectBall::Create()
{
	auto pInstance = E::ToUPtr(new CMonEffectBall{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CMonEffectBall");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CMonEffectBall::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CMonEffectBall{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMonEffectBall");
		return nullptr;
	}

	return pInstance;
}
