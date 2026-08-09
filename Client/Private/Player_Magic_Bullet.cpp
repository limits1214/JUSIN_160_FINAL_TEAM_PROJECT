#include "pch.h"
#include "Player_Magic_Bullet.h"
#include "Client_Resources.h"
#include "Trail_CPU.h"
#include "PhysXManager.h"
#include "Light.h"

#include "Monster.h"
#include "BossTMB.h"

#include "ComSound.h"
NS_USING(Client)

CPlayer_Magic_Bullet::CPlayer_Magic_Bullet()
	: CGameObject{}
{
}


CPlayer_Magic_Bullet::~CPlayer_Magic_Bullet()
{
	StopFlightSound();

	if (!m_hPointLight)
		return;

	auto* pPointLight = CGameInstance::Get().GetGameObjectByHandleT<CLight>(
		m_hPointLight.value());
	if (pPointLight)
		pPointLight->Reset_Light();
}

void CPlayer_Magic_Bullet::UpdateGUI()
{
	CGameObject::UpdateGUI();


}

HRESULT CPlayer_Magic_Bullet::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CPlayer_Magic_Bullet::Initialize(void* pArg)
{
	if (nullptr == pArg)
		return E_FAIL;

	const auto pDesc = static_cast<const MAGIC_BULLET_DESC*>(pArg);
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	m_vStartPosition = pDesc->vStartPosition;
	m_vEndPosition = pDesc->vEndPosition;
	m_fSpeed = pDesc->fSpeed;
	m_fRadius = pDesc->fRadius;
	m_hOwner = pDesc->hOwner;
	m_tQueryFilter = pDesc->tQueryFilter;
	m_tQueryFilter.hIgnoreGameObject = m_hOwner;

	if (m_fSpeed <= 0.f || m_fRadius <= 0.f)
		return E_FAIL;

	BuildSpline(pDesc->fCurveHeight, pDesc->iSampleCount);
	if (m_Splines.empty())
		return E_FAIL;

	GetTransform().SetPosition(m_Splines.front());
	GetTransform().Update();

	CComSound::DESC tSoundDesc{};
	if (FAILED(AddComponentFromProto(
		ES_EngineProtoMajorType::PERMANENT,
		ES_EngineProtoComponent::Prototype_Component_ComSound,
		"Com_Sound",
		&tSoundDesc,
		&m_pComSound)))
		return E_FAIL;

	m_pComSound->PlaySlot3D(
		E::StringID{ "PLAYER_MAGIC_BULLET_FLIGHT" },
		"./Resources/SampleClient/Sound/Player/SkillEffect/BasicAttack/BasicAttack_Projectile_01.wav",
		SOUND_3D_DESC{
			.vPosition = GetTransform().GetPosition(),
			.fMinDistance = 1.f,
			.fMaxDistance = 18.f,
			.eRolloff = SOUND_3D_ROLLOFF::LINEAR
		},
		SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::SFX,
			.fVolume = 0.1f,
			.fPitch = 1.f,
			.iPriority = 72,
			.bLoop = true
		});

	m_hPointLight = CGameInstance::Get().Allocate_EffectLight(
		GetTransform().GetLoadedPostion(),
		80.f,
		{ 0.35f, 0.65f, 1.f },
		4.6f,
		6.f,
		99999.f,
		{ 0.f, 0.f, 0.f });
	return S_OK;
}

void CPlayer_Magic_Bullet::PriorityUpdate(E::_float fTimeDelta)
{
}

void CPlayer_Magic_Bullet::FixedUpdate(E::_float fTimeDelta)
{
	if (fTimeDelta <= 0.f || m_Splines.size() < 2 ||
		m_iSplineIndex >= m_Splines.size() - 1)
		return;

	_float fRemainDistance = m_fSpeed * fTimeDelta;
	while (fRemainDistance > 0.f && m_iSplineIndex < m_Splines.size() - 1)
	{
		const _vector vCurrent = XMLoadFloat3(&m_Splines[m_iSplineIndex]);
		const _vector vNext = XMLoadFloat3(&m_Splines[m_iSplineIndex + 1]);
		const _float fSegmentLength = XMVectorGetX(XMVector3Length(vNext - vCurrent));
		const _float fSegmentRemain = fSegmentLength - m_fDistanceOnSegment;

		if (fSegmentLength <= 0.0001f)
		{
			++m_iSplineIndex;
			m_fDistanceOnSegment = 0.f;
			continue;
		}

		const _float fMoveDistance = std::min(fRemainDistance, fSegmentRemain);
		const _float fStartRatio = m_fDistanceOnSegment / fSegmentLength;
		const _float fEndRatio =
			(m_fDistanceOnSegment + fMoveDistance) / fSegmentLength;

		_float3 vMoveStart{};
		_float3 vMoveEnd{};
		XMStoreFloat3(&vMoveStart, XMVectorLerp(vCurrent, vNext, fStartRatio));
		XMStoreFloat3(&vMoveEnd, XMVectorLerp(vCurrent, vNext, fEndRatio));

		PX_SWEEP_RESULT tHit{};
		if (SweepSegment(vMoveStart, vMoveEnd, tHit))
		{
			const _vector vDirection = XMVector3Normalize(
				XMLoadFloat3(&vMoveEnd) - XMLoadFloat3(&vMoveStart));
			_float3 vHitCenter{};
			XMStoreFloat3(
				&vHitCenter,
				XMLoadFloat3(&vMoveStart) + vDirection * tHit.fDistance);
			GetTransform().SetPosition(vHitCenter);
			GetTransform().Update();
			HandleSweepHit(tHit);
			return;
		}

		GetTransform().SetPosition(vMoveEnd);
		m_fDistanceOnSegment += fMoveDistance;
		fRemainDistance -= fMoveDistance;

		if (m_fDistanceOnSegment >= fSegmentLength - 0.0001f)
		{
			++m_iSplineIndex;
			m_fDistanceOnSegment = 0.f;
		}
	}

	if (m_iSplineIndex >= m_Splines.size() - 1)
	{
		StopFlightSound();
		SetPendingDestroy();
	}
}

void CPlayer_Magic_Bullet::UpdatePointLight()
{
	if (!m_hPointLight)
		return;

	auto* pPointLight = CGameInstance::Get().GetGameObjectByHandleT<CLight>(
		m_hPointLight.value());
	if (nullptr == pPointLight)
	{
		m_hPointLight.reset();
		return;
	}

	pPointLight->Set_LightPosition(GetTransform().GetLoadedPostion());
}

void CPlayer_Magic_Bullet::Update(E::_float fTimeDelta)
{
	UpdatePointLight();
	if (m_pComSound)
	{
		m_pComSound->SetSlot3DAttributes(
			E::StringID{ "PLAYER_MAGIC_BULLET_FLIGHT" },
			GetTransform().GetPosition());
		m_pComSound->Update();
	}

	{
		_float3 vstart, vend;

		vstart = _float3(m_pComTransform->GetPosition().x, m_pComTransform->GetPosition().y + 0.4f, m_pComTransform->GetPosition().z);
		vend = _float3(m_pComTransform->GetPosition().x, m_pComTransform->GetPosition().y - 0.4f, m_pComTransform->GetPosition().z);
		CGameInstance::Get().AddTrailPoint("PlayerAttackTrail_CPU", "PlayerAttackTrail_CPU", vstart, vend);
	}
}

void CPlayer_Magic_Bullet::LateUpdate(E::_float fTimeDelta)
{
	GetTransform().Update();

	//auto matrix = XMLoadFloat4x4(m_pComTransform->GetWorldMatrix());

	//auto cachedCol = CGameInstance::Get().GetDbgLineRender()->GetColor();
	//auto cachedDepth = CGameInstance::Get().GetDbgLineRender()->GetDepthMode();
	//CGameInstance::Get().GetDbgLineRender()->SetColor({ 1.f, 0.f, 0.f, 1.f });
	//CGameInstance::Get().GetDbgLineRender()->SetDepthTest(false);
	//CGameInstance::Get().GetDbgLineRender()->AddSphere(m_fRadius, matrix);
	//CGameInstance::Get().GetDbgLineRender()->SetColor(cachedCol);
	//CGameInstance::Get().GetDbgLineRender()->SetDepthMode(cachedDepth);
}

HRESULT CPlayer_Magic_Bullet::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

_bool CPlayer_Magic_Bullet::SweepSegment(
	const _float3& vStart,
	const _float3& vEnd,
	PX_SWEEP_RESULT& tHit) const
{
	const _vector vDisplacement =
		XMLoadFloat3(&vEnd) - XMLoadFloat3(&vStart);
	const _float fDistance =
		XMVectorGetX(XMVector3Length(vDisplacement));
	if (fDistance <= 0.0001f)
		return false;

	_float3 vDirection{};
	XMStoreFloat3(
		&vDirection,
		XMVector3Normalize(vDisplacement));

	PX_SWEEP_DESC tSweep{};
	tSweep.tGeometry.eType = PX_QUERY_GEOMETRY_TYPE::SPHERE;
	tSweep.tGeometry.fRadius = m_fRadius;
	tSweep.tPose.vPosition = vStart;
	tSweep.vDirection = vDirection;
	tSweep.fMaxDistance = fDistance;
	tSweep.tFilter = m_tQueryFilter;

	auto* pPhysXManager =
		CGameInstance::Get().GetPhysXManager();
	return pPhysXManager &&
		pPhysXManager->Sweep(tSweep, tHit) &&
		tHit.bHit;
}

void CPlayer_Magic_Bullet::HandleSweepHit(
	const PX_SWEEP_RESULT& tHit)
{
	StopFlightSound();
	DEBUG_LOG_STR(std::string("[PX][CPlayer_Magic_Bullet] Sweep Hit : ") +
		(tHit.pGameObject ?
			std::string{ tHit.pGameObject->GetObjectTag() } :
			"null") + "\n");
	_float4x4 tImpactWorld{};
	auto camera = CGameInstance::Get().GetActiveCamera();
	if (camera)
	{
		const XMVECTOR hitPosition = XMLoadFloat3(&tHit.vHitpos);
		const XMVECTOR cameraPosition = camera->GetTransform().GetLoadedPostion();
		const XMVECTOR look = XMVector3Normalize(cameraPosition - hitPosition);
		XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
		XMVECTOR right = XMVector3Cross(up, look);
		if (XMVectorGetX(XMVector3LengthSq(right)) < 0.0001f)
			right = XMVectorSet(1.f, 0.f, 0.f, 0.f);
		else
			right = XMVector3Normalize(right);
		up = XMVector3Normalize(XMVector3Cross(look, right));
		XMMATRIX impactWorld = XMMatrixIdentity();
		impactWorld.r[0] = right;
		impactWorld.r[1] = up;
		impactWorld.r[2] = look;
		impactWorld.r[3] = XMVectorSetW(hitPosition, 1.f);
		_float4x4 impactWorldData{};
		XMStoreFloat4x4(&impactWorldData, impactWorld);
		CGameInstance::Get().PlayEffect("PlayerAttackSpread", impactWorldData);
	}
	if (auto pTmbGurdian= Cast<CMonster>(tHit.pGameObject))
	{
	/*	static constexpr const char* HIT_SOUND_PATHS[] =
		{
			"./Resources/SampleClient/Sound/avada.wav",
	
		};
		constexpr int HIT_SOUND_COUNT = static_cast<int>(sizeof(HIT_SOUND_PATHS) / sizeof(HIT_SOUND_PATHS[0]));
		const int iSoundIndex = Engine::RandInt(0, HIT_SOUND_COUNT - 1);

		auto id = CGameInstance::Get().GetSoundManager()->Play3D(
			HIT_SOUND_PATHS[iSoundIndex],
			SOUND_3D_DESC{
				.vPosition = tHit.vHitpos,
				.fMinDistance = 10.f,
				.fMaxDistance = 30.f,
				.eRolloff = SOUND_3D_ROLLOFF::LINEAR
			},
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::SFX,
				.fVolume = 1.f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			}
		);*/
		//if (id == INVALID_SOUND_ID)
		//{
		//	MSG_BOX("INVALID_SOUND_ID");
		//}

		pTmbGurdian->Check_Table(PLAYER_SKILL_TYPE::ATTACK);
	}

	SetPendingDestroy();
}

void CPlayer_Magic_Bullet::StopFlightSound()
{
	if (m_pComSound)
		m_pComSound->StopSlot(E::StringID{ "PLAYER_MAGIC_BULLET_FLIGHT" });
}

void CPlayer_Magic_Bullet::BuildSpline(_float fCurveHeight, uint32_t iSampleCount)
{
	m_Splines.clear();
	m_iSplineIndex = 0;
	m_fDistanceOnSegment = 0.f;

	iSampleCount = std::max(iSampleCount, 3u);

	const _vector vStart = XMLoadFloat3(&m_vStartPosition);
	const _vector vEnd = XMLoadFloat3(&m_vEndPosition);

	_vector vForward = vEnd - vStart;
	const _float fDistance = XMVectorGetX(XMVector3Length(vForward));

	if (fDistance <= 0.0001f)
		return;

	vForward = XMVector3Normalize(vForward);

	// 진행 방향과 수직인 축 생성
	_vector vRight = XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), vForward);

	if (XMVectorGetX(XMVector3LengthSq(vRight)) <= 0.0001f)
		vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	else
		vRight = XMVector3Normalize(vRight);

	const _vector vUp = XMVector3Normalize(XMVector3Cross(vForward, vRight));

	// 발사할 때 한 번만 랜덤 결정
	const _float fPhase = XMConvertToRadians((_float)(rand() % 360));

	const _float fFrequency = 1.5f + (_float)(rand() % 150) / 100.f;

	const _float fRightWeight = (rand() % 2 == 0) ? 1.f : -1.f;

	m_Splines.reserve(iSampleCount + 1);

	for (uint32_t i = 0; i <= iSampleCount; ++i)
	{
		const _float t = static_cast<_float>(i) / iSampleCount;

		// 기본 직선 위치
		_vector vPosition = XMVectorLerp(vStart, vEnd, t);

		// 시작점과 도착점에서 흔들림이 반드시 0이 되게 함
		const _float fEnvelope = std::sin(XM_PI * t);

		const _float fWave = std::sin(XM_2PI * fFrequency * t + fPhase);

		const _float fSecondWave = std::sin(XM_2PI * (fFrequency * 0.7f) * t + fPhase * 0.5f);

		vPosition += vRight * fWave * fCurveHeight * fEnvelope * fRightWeight;

		vPosition += vUp * fSecondWave * fCurveHeight * 0.5f * fEnvelope;

		_float3 vStoredPosition{};
		XMStoreFloat3(&vStoredPosition, vPosition);
		m_Splines.push_back(vStoredPosition);
	}

}

E::UPtr<CPlayer_Magic_Bullet> CPlayer_Magic_Bullet::Create()
{
	auto pInstance = E::ToUPtr(new CPlayer_Magic_Bullet{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CPlayer_Magic_Bullet");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CPlayer_Magic_Bullet::Clone(void* pArg)
{
	auto pInstance = E::ToUPtr(new CPlayer_Magic_Bullet{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CPlayer_Magic_Bullet");
		return nullptr;
	}

	return pInstance;
}
