#include "pch.h"
#include "StarBurst.h"
#include "Client_Resources.h"
#include "Trail_CPU.h"

#include "BossTMB.h"
#include "Player.h"

#include "ComBeHavior.h"
NS_USING(Client)

CBoss_StarBurst::CBoss_StarBurst()	: CGameObject{}	{}
CBoss_StarBurst::~CBoss_StarBurst()					{}

void CBoss_StarBurst::UpdateGUI() {
	CGameObject::UpdateGUI();
}

HRESULT CBoss_StarBurst::InitializePrototype(void* pArg) {
	m_pLavaFlame_SpawnInterval = 0.05f;
	m_pLavaFlame_CastingTime = 2.0f;
	m_pLavaFlame_StayTime = 1.0f;

	return S_OK;
}
HRESULT CBoss_StarBurst::Initialize(void* pArg) {
	if (nullptr == pArg)	return E_FAIL;

	if (FAILED(CGameObject::Initialize(pArg)))	return E_FAIL;

	const auto pDesc = static_cast<const STARBURST_DESC*>(pArg);
	if (pDesc->fSpeed <= 0.f || pDesc->fRadius <= 0.f)
		return E_INVALIDARG;
	m_iDamage = 30.f;
	m_pTargetHandle = pDesc->pTargetHandle;
	m_fSpeed = pDesc->fSpeed;
	m_fRadius = pDesc->fRadius;
	m_tQueryFilter = pDesc->tQueryFilter;
	m_hOwner = pDesc->hOwner;
	GetTransform().SetPosition(pDesc->vStartPosition);
	GetTransform().Update();

	m_pLightEffectID = CGameInstance::Get().PlayEffect("Boss_StarBurst_A", *GetTransform().GetWorldMatrix(), _vector{},
		[h = GetHandle()](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
		{
			if (auto pOBJ = CGameInstance::Get().GetGameObjectByHandleT<CBoss_StarBurst>(h)) {
				pOBJ->m_pLightEffectID = INVALID_EFFECT_INSTANCE_ID;
			}
		});	

	return S_OK;
}

void CBoss_StarBurst::PriorityUpdate(E::_float fTimeDelta)
{
}

void CBoss_StarBurst::FixedUpdate(E::_float fTimeDelta)
{
	if (fTimeDelta <= 0.f ||
		m_pLightEffectID == INVALID_EFFECT_INSTANCE_ID)
	{
		return;
	}

	m_fEffectLifeTime += fTimeDelta;

	if (m_fEffectLifeTime <= m_pLavaFlame_CastingTime)
	{
		Translate_Casting(
			m_fEffectLifeTime /
			m_pLavaFlame_CastingTime);
	}
	else if (m_fEffectLifeTime <
		m_pLavaFlame_CastingTime +
		m_pLavaFlame_StayTime)
	{
		
	}
	else {
		Translate_Attacking(fTimeDelta);
	}
}

void CBoss_StarBurst::Update(E::_float fTimeDelta) {

	Dead_Check(fTimeDelta);
	if (m_pLightEffectID == INVALID_EFFECT_INSTANCE_ID || m_bDead || m_fDeadTick >5.f)  {
		SetPendingDestroy();
		return;
	}

	m_fEffectSpawnTimer += fTimeDelta;

	if (m_fEffectSpawnTimer >= m_pLavaFlame_SpawnInterval) {
		m_fEffectSpawnTimer = 0.f;
		CGameInstance::Get().PlayEffect("Boss_StarBurst_B", *GetTransform().GetWorldMatrix(), XMVECTOR{});
	}

}

void CBoss_StarBurst::LateUpdate(E::_float fTimeDelta) {
	GetTransform().Update();

	auto matrix = XMLoadFloat4x4(m_pComTransform->GetWorldMatrix());

	auto cachedCol = CGameInstance::Get().GetDbgLineRender()->GetColor();
	auto cachedDepth = CGameInstance::Get().GetDbgLineRender()->GetDepthMode();
	CGameInstance::Get().GetDbgLineRender()->SetColor({ 1.f, 0.f, 0.f, 1.f });
	CGameInstance::Get().GetDbgLineRender()->SetDepthTest(false);
	CGameInstance::Get().GetDbgLineRender()->AddSphere(m_fRadius, matrix);
	CGameInstance::Get().GetDbgLineRender()->SetColor(cachedCol);
	CGameInstance::Get().GetDbgLineRender()->SetDepthMode(cachedDepth);


}
HRESULT CBoss_StarBurst::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {

	return S_OK;
}

void CBoss_StarBurst::Translate_Casting(_float fRatio){

	_float3 LocalDestination = { 16.f, 10.f, 2.f };


	m_fCurrEffectMovementValue = 1.f - pow(1.f - fRatio, 3.f);
	_float DeltaMovementValue = m_fCurrEffectMovementValue - m_fPrevEffectMovementValue;
	m_fPrevEffectMovementValue = m_fCurrEffectMovementValue;

	const _float3 CurrentPosition = GetTransform().GetPosition();
	MoveWithSweep(XMFLOAT3(
		CurrentPosition.x + DeltaMovementValue * LocalDestination.x, 
		CurrentPosition.y + DeltaMovementValue * LocalDestination.y,
		CurrentPosition.z + DeltaMovementValue * LocalDestination.z));

}

void CBoss_StarBurst::Translate_Attacking(_float fTimeDelta) {
	auto GamePlayer = CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(m_pTargetHandle);
	if (nullptr == GamePlayer)  return;

	const _float3 CurrentPosition = GetTransform().GetPosition();
	if (XMVectorGetX(XMVectorEqual(XMVectorZero(), m_vDirection))) {
		m_vDirection = XMVector3Normalize(GamePlayer->GetTransform().GetLoadedPostion() + XMVectorSet(0.f, 1.5f, 0.f, 0.f) - XMLoadFloat3(&CurrentPosition));
	}

	_float3 EffectPos{};
	XMStoreFloat3(
		&EffectPos,
		XMLoadFloat3(&CurrentPosition) +
		m_vDirection * m_fSpeed * fTimeDelta);

	MoveWithSweep(EffectPos);
}

_bool CBoss_StarBurst::MoveWithSweep(
	const _float3& vNextPosition)
{
	const _float3 vCurrentPosition =
		GetTransform().GetPosition();
	const _vector vDisplacement =
		XMLoadFloat3(&vNextPosition) -
		XMLoadFloat3(&vCurrentPosition);
	const _float fDistance =
		XMVectorGetX(XMVector3Length(vDisplacement));

	if (fDistance <= FLT_EPSILON)
		return true;

	_float3 vDirection{};
	XMStoreFloat3(
		&vDirection,
		XMVector3Normalize(vDisplacement));

	PX_SWEEP_DESC tSweep{};
	tSweep.tGeometry.eType =
		PX_QUERY_GEOMETRY_TYPE::SPHERE;
	tSweep.tGeometry.fRadius = m_fRadius;
	tSweep.tPose.vPosition = vCurrentPosition;
	tSweep.vDirection = vDirection;
	tSweep.fMaxDistance = fDistance;
	tSweep.tFilter = m_tQueryFilter;

	PX_SWEEP_RESULT tHit{};
	auto* pPhysXManager =
		CGameInstance::Get().GetPhysXManager();
	if (pPhysXManager &&
		pPhysXManager->Sweep(tSweep, tHit) &&
		tHit.bHit &&
		HandleSweepHit(tHit))
	{
		_float3 vHitCenter{};
		XMStoreFloat3(
			&vHitCenter,
			XMLoadFloat3(&vCurrentPosition) +
			XMLoadFloat3(&vDirection) *
			tHit.fDistance);
		GetTransform().SetPosition(vHitCenter);
		GetTransform().Update();
		return false;
	}

	GetTransform().SetPosition(vNextPosition);
	GetTransform().Update();
	CGameInstance::Get().SetEffectPosition(
		m_pLightEffectID,
		vNextPosition);

	return true;
}

_bool CBoss_StarBurst::HandleSweepHit(
	const PX_SWEEP_RESULT& tHit)
{
	DEBUG_LOG_STR(std::string(
		"[PX][CBoss_StarBurst] Sweep Hit : ") +
		(tHit.pGameObject ?
			std::string{ tHit.pGameObject->GetObjectTag() } :
			"null") + "\n");

	if (auto pPlayer = Cast<CPlayer>(tHit.pGameObject))
	{
		CGameInstance::Get().StopEffect(m_pLightEffectID);

		_float3 EffectScale = { 1.f, 1.f, 1.f };
		XMMATRIX ScaleMatrix = XMMatrixScaling(EffectScale.x, EffectScale.y - 0.5f, EffectScale.z);

		const _float3 EffectPosition = tHit.vHitpos;
		XMMATRIX PositionMat = XMMatrixTranslation(EffectPosition.x, EffectPosition.y, EffectPosition.z);

		XMVECTOR Forward = m_vDirection;
		if (XMVectorGetX(XMVector3LengthSq(Forward)) <= FLT_EPSILON)
			Forward = XMVectorSet(0.f, 0.f, 1.f, 0.f);
		else
			Forward = XMVector3Normalize(Forward);
		XMVECTOR WorldUP = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

		_float DDU = XMVectorGetX(XMVectorAbs(XMVector3Dot(Forward, WorldUP)));
		if (DDU > 0.999f) WorldUP = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

		XMVECTOR Right = XMVector3Normalize(XMVector3Cross(WorldUP, Forward));
		XMVECTOR Up = XMVector3Cross(Forward, Right);

		XMMATRIX RotationMatrix = XMMATRIX(Right, Up, Forward, XMVectorSet(0.f, 0.f, 0.f, 1.f));
		RotationMatrix = RotationMatrix * XMMatrixRotationY(XMConvertToRadians(180.f));

		XMFLOAT4X4 WorldMatrix{};
		XMStoreFloat4x4(&WorldMatrix, ScaleMatrix * RotationMatrix * PositionMat);
		if (auto pPlayer = Cast<CPlayer>(tHit.pGameObject))
		{
			pPlayer->OnQueryHit(m_iDamage, tHit.vHitpos);

			CGameInstance::Get().StopEffect(m_pLightEffectID);

			// 기존 피격 이펙트 코드
			SetPendingDestroy();
			return true;
		}
		auto EffectID = CGameInstance::Get().PlayEffect("Boss_HitSplash", WorldMatrix, XMVECTOR{});
		CGameInstance::Get().SetEffectPosition(EffectID, EffectPosition);
		SetPendingDestroy();
		return true;
	}

	return false;
}

void CBoss_StarBurst::Dead_Check(_float fTimeDelta)
{
	m_fDeadTick += fTimeDelta;

	if (auto iter = CGameInstance::Get().GetGameObjectByHandle(m_hOwner))
	{
		if (auto pBT = iter->GetComponent<CComBeHavior>("Com_BT"))
		{
			if (pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::NOCKDOWN) | ETOUI(CBTRoot::BTFLAG::GROGY) | ETOUI(CBTRoot::BTFLAG::DEAD)))
			{
				CGameInstance::Get().StopEffect(m_pLightEffectID);
				m_bDead = true;
				return;
			}

		}
	}
}

E::UPtr<CBoss_StarBurst>		CBoss_StarBurst::Create()
{
	auto pInstance = E::ToUPtr(new CBoss_StarBurst{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBoss_StarBurst");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CBoss_StarBurst::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CBoss_StarBurst{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBoss_StarBurst");
		return nullptr;
	}

	return pInstance;
}
