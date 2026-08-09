#include "pch.h"
#include "BossTMB.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "DbgLineRender.h"
#include "StarBurst.h"
#include "MonEffectBall.h"
#include "BossMace.h"
#include "UIController.h"
#include "UIManager.h"
#include "Player.h"
NS_USING(Client)

CBossTMB::CBossTMB()
{
}

CBossTMB::~CBossTMB()
{
}

void CBossTMB::UpdateGUI()
{
	__super::UpdateGUI();
	
	ImGui::Separator();
	ImGui::Text(true == m_bStar ? "TRUE" : "FALSE");
}

HRESULT CBossTMB::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	return S_OK;
}

HRESULT CBossTMB::Initialize(void* pArg)
{
	auto MonDesc = static_cast<MONSTER_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}
	m_iHp = m_iMaxHp = 500;

	{
		CComPxCharacterController::DESC Desc{};
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		const _float fHorizontalScale =
			std::max(std::abs(MonDesc->vScale.x), std::abs(MonDesc->vScale.z));
		const _float fVerticalScale = std::abs(MonDesc->vScale.y);
		const _float3 vCenterOffset{
			MonDesc->vCCTCenterOffset.x * MonDesc->vScale.x,
			MonDesc->vCCTCenterOffset.y * fVerticalScale,
			MonDesc->vCCTCenterOffset.z * MonDesc->vScale.z };
		Desc.fHeight = MonDesc->fCCTHeight * fVerticalScale;
		Desc.fRadius = MonDesc->fCCTRadius * fHorizontalScale;
		Desc.fStepOffset = MonDesc->fCCTStepOffset;
		Desc.vPosition = {
			MonDesc->vPos.x + vCenterOffset.x,
			MonDesc->vPos.y + vCenterOffset.y,
			MonDesc->vPos.z + vCenterOffset.z };
		Desc.tFilter = MonDesc->tFilter;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCharacterController,
			"ComPxCharacterController", &Desc, &m_pCharacterController)))
		{
			return E_FAIL;
		}
	}
	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody, "ComPxRigidBody", &Desc, &m_pComRigidBody)))
		{
			MSG_BOX("Create Failed ComPxRigidBody TombGurdian");
			return E_FAIL;
		}
	}

	{
		CComPxSphereCollider::DESC Desc{};
		Desc.pComPxRigidBody = m_pComRigidBody;
		Desc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		Desc.bIsTrigger = false;
		Desc.tFilter = PX_FILTER_DESC{
			.iLayer = ETOUI(COLLISION_LAYER::ENEMY_HURTBOX),
			.iSimulationMask = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE),
			//.iQueryMask = ETOUI(COLLISION_LAYER::PLAYER_PROJECTILE),
		};
		Desc.pResSphereGeo = CResPhysXSphereGeometry::CreateAndLoad({ .fRadius = 1.2f });
		if (!Desc.pResMaterial ||
			FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxSphereCollider,
				"ComPxSphereCollider", &Desc, &m_pComSphereCol)))
		{
			MSG_BOX("Create Failed ComPxSphereCollider TmbGurdian");
			return E_FAIL;
		}
		if (!m_pComSphereCol->SetQueryEnabled(false))
			return E_FAIL;
	}
	{
		CComCharacterMoveIntent::DESC Desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMoveIntent,
			"ComCharacterMoveIntent", &Desc, &m_pMoveIntent)))
		{
			return E_FAIL;
		}
	}

	{
		CComCharacterMotor::DESC Desc{};
		Desc.pMoveIntent = m_pMoveIntent;
		Desc.pCharacterController = m_pCharacterController;
		Desc.fGravity = -9.81f;
		Desc.vControllerCenterOffset = {
			MonDesc->vCCTCenterOffset.x * MonDesc->vScale.x,
			MonDesc->vCCTCenterOffset.y * std::abs(MonDesc->vScale.y),
			MonDesc->vCCTCenterOffset.z * MonDesc->vScale.z };
		Desc.bUseGravity = true;
		Desc.bSyncTransform = true;
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComCharacterMotor,
			"ComCharacterMotor", &Desc, &m_pCharacterMotor)))
		{
			return E_FAIL;
		}
	}

	CComBeHavior::BEHAVIOR_DESC Desc{};
	Desc.OwnerName = "Com_BT";
	Desc.LoadPath = MonDesc->BeHaviorTag;
	Desc.resBeHaviorMajor = MonDesc->resBeHaviorMajor;
	Desc.resBeHaviorMinor = MonDesc->resBeHaviorMinor;
	if (FAILED(AddComponentFromProto("BEHAVIOR", "Prototype_Component_BeHavior", "Com_BT", &Desc, &m_pBeHavior)))
	{
		return E_FAIL;
	};
	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		};
	}

	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = MonDesc->LevelTag;
		Desc.sResTag = MonDesc->ReSourceTag;

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ModelInstance", "ComCModelIntance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		};
	}

	{
		CComAnimator::DESC DescAnim{};
		DescAnim.sComTag = "ComCModelIntance";

		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_Animator", "ComCModelAnimator", &DescAnim, &m_pModelAnimator)))
		{
			return E_FAIL;
		};
	}

	{
		CComCollider::DESC Desc{};
		Desc.eCollType = CollType::Box;
		Desc.vExtents = { 1.f, 1.f, 1.f };
		if (FAILED(AddComponentFromProto("COLLIDER", "Prototype_Component_Collider", "ComColl", &Desc, &m_pComCollider)))
		{
			return E_FAIL;
		};
	}
	CMon_Weapon::WEAPON_DESC WeaponDesc{};
	if (MonDesc->WeaponResourceName != "")
	{

		WeaponDesc.sObjectTag = "Weapon";
		WeaponDesc.ParentHandle = GetHandle();
		WeaponDesc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_FX_Reference1Socket");
		WeaponDesc.WeaponName = MonDesc->WeaponResourceName;
		WeaponDesc.LevelTag = MonDesc->LevelTag;
		WeaponDesc.vScale = MonDesc->vWeaponScale;
		auto Weapon = E::CGameInstance::Get().AddGameObjectToLayer(MonDesc->LevelTag, MonDesc->WeaponProtoName, "03_Weapon", &WeaponDesc);
		if (!Weapon.has_value())
		{
			MSG_BOX("Create Failed Weapon To BossTmb");
			return E_FAIL;
		}
		m_Partes[ETOUI(PARTES::WEAPON)] = Weapon.value();
	}

	m_MonSkillLists[ATTMON::SLOT0] = ETOUI(BOSSTOMB_SKILL::SPAWN);
	m_MonSkillLists[ATTMON::SLOT1] = ETOUI(BOSSTOMB_SKILL::STUMP);
	m_MonSkillLists[ATTMON::SLOT2] = ETOUI(BOSSTOMB_SKILL::BLUST_START);
	m_MonSkillLists[ATTMON::SLOT3] = ETOUI(BOSSTOMB_SKILL::BLUST_END);
	m_MonSkillLists[ATTMON::SLOT4] = ETOUI(BOSSTOMB_SKILL::BALL);
	m_MonSkillLists[ATTMON::SLOT5] = ETOUI(BOSSTOMB_SKILL::BALL_BREAK);
	m_MonSkillLists[ATTMON::SLOT6] = ETOUI(BOSSTOMB_SKILL::SMESH);
	m_MonSkillLists[ATTMON::SLOT7] = ETOUI(BOSSTOMB_SKILL::DEAD);

	m_MonSkillLists[ATTMON::SKIP] = ETOUI(BOSSTOMB_SKILL::SKIP);
	m_EffectNames[ETOUI(BOSSTOMB_SKILL::SPAWN)] = "Boss_Appear";
	m_EffectNames[ETOUI(BOSSTOMB_SKILL::STUMP)] = "Boss_GroundCrash";
	m_EffectNames[ETOUI(BOSSTOMB_SKILL::BLUST_START)] = "BossAoeBlustStart";
	m_EffectNames[ETOUI(BOSSTOMB_SKILL::BLUST_END)] = "BossAoeBlustEnd";
	m_EffectNames[ETOUI(BOSSTOMB_SKILL::SMESH)] = "MorningStarAfterEffect";
	m_EffectNames[ETOUI(BOSSTOMB_SKILL::BALL)] = "BossRingAttack";
	m_EffectNames[ETOUI(BOSSTOMB_SKILL::DEAD)] = "Boss_Dead";

	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();

	m_pComTransform->SetRotation(XMVectorSet(MonDesc->vRot.x, MonDesc->vRot.y, MonDesc->vRot.z, 0.f), MonDesc->fAngle);
	m_pComTransform->SetScale(XMVectorSet(MonDesc->vScale.x, MonDesc->vScale.y, MonDesc->vScale.z, 0));
	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pModelAnimator->Build_BoneMatrices_CPU(0.f);


	m_eMonType = MONSTER_TYPE::BOSS;
	m_eAttType = ATTMON::END;
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
	m_fEMissiveColor = { 0.75f,0.9f,1.f};

	/*----------- 광윤 추가 -----------*/
	// 보스가 빛을 등지면 너무 어두워져서 전면만 추가 라이트 설치
	AdditionalLightHandle = CGameInstance::Get().Allocate_EffectLight(GetTransform().GetLoadedPostion(), 500.f, { 0.47f, 1.f, 1.f }, 15.f, 20.f, 99999.f, { 0.f, 0.f, 0.f });
	/*---------------------------------*/
	m_iColliderBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("Spine1");
	return S_OK;
}



const _float CBossTMB::Get_Damage()
{
	uint32_t SkillID = Find_SkillNum(m_eAttType);

	if (SkillID == ETOUI(BOSSTOMB_SKILL::BLUST_END))
	{
		m_fDamage = 45.f;
	}else if (SkillID == ETOUI(BOSSTOMB_SKILL::STUMP))
	{
		m_fDamage = 25.f;
	}


	return m_fDamage;
}

void CBossTMB::OverLabTest(_vector vSrcPos, _float fRadius, int32_t iDamage)
{
	_float3 vSweepPos = {};
	XMStoreFloat3(&vSweepPos, vSrcPos);
	PX_OVERLAP_DESC   pxOverLabDesc{};
	PX_OVERLAP_RESULT pxOverLapResult{};

	pxOverLabDesc.tFilter = PX_QUERY_FILTER_DESC{ .iQueryMask = ETOUI(COLLISION_LAYER::PLAYER_HURTBOX) };
	pxOverLabDesc.tGeometry = PX_QUERY_GEOMETRY_DESC{ .eType = PX_QUERY_GEOMETRY_TYPE::SPHERE,.fRadius = fRadius };
	pxOverLabDesc.tPose = PX_QUERY_POSE{ .vPosition = vSweepPos };

	_float3 vPos = pxOverLabDesc.tPose.vPosition;
	auto pDbgLineRender = CGameInstance::Get().GetDbgLineRender();

	const auto vPreviousColor = pDbgLineRender->GetColor();
	const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
	pDbgLineRender->SetColor({ 0.f, 1.f, 1.f, 1.f });
	pDbgLineRender->SetDepthTest(true);
	pDbgLineRender->AddSphere(fRadius, XMMatrixTranslation(vSweepPos.x, vSweepPos.y, vSweepPos.z));
	pDbgLineRender->SetColor(vPreviousColor);
	pDbgLineRender->SetDepthMode(ePreviousDepthMode);

	if (CGameInstance::Get().GetPhysXManager()->Overlap(pxOverLabDesc, pxOverLapResult))
	{
		if (pxOverLapResult.bHit)
		{
			//m_fDamage
			auto pTarget = CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(pxOverLapResult.hGameObject);
			pTarget->OnQueryHit(iDamage);
		}
	}
}

void CBossTMB::Active_Skill()
{
	
	if (m_eAttType == ATTMON::END)
		return;
	if (Find_SkillNum(m_eAttType) == ETOUI(BOSSTOMB_SKILL::BALL) || Find_SkillNum(m_eAttType) == ETOUI(BOSSTOMB_SKILL::SMESH))
		return;
	if (m_iCurSkill == m_iPreSkill)
		return;

	if (Check_Flag(ETOUI(CBTRoot::BTFLAG::LOOP)))
	{
		if (m_iCurEffectID != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().SetEffectWorldMatrix(m_iCurEffectID, *GetTransform().GetWorldMatrix());
		m_bSkillLoop = true;
	}
		
	_float fCurrRatio = m_pModelAnimator->GetPlayAnimRatio();
	
	if (!Check_Flag(ETOUI(CBTRoot::BTFLAG::ATTACK)) && fCurrRatio >= m_fSkillRatio.x && fCurrRatio < m_fSkillRatio.y)
	{

		auto k = GetTransform().GetWorldMatrix();
		
			m_iCurEffectID = CGameInstance::Get().PlayEffect(m_CurEffectName, *GetTransform().GetWorldMatrix(), _vector{},
				[this](EFFECT_INSTANCE_ID effectId, EFFECT_FINISH_REASON reason)
				{
					if (effectId != m_iCurEffectID)
						return;
					m_iCurEffectID = INVALID_EFFECT_INSTANCE_ID;
				});
		
		m_iPreSkill = m_iCurSkill;
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::ATTACK), FLAGTYPE::ADD);
	}
}
void CBossTMB::PriorityUpdate(E::_float fTimeDelta)
{
	
	__super::PriorityUpdate(fTimeDelta);

	if (Check_Flag(ETOUI(CBTRoot::BTFLAG::DEAD)))
	{

		if (m_CurEffectName == m_EffectNames[ETOUI(BOSSTOMB_SKILL::BLUST_START)] || m_CurEffectName == m_EffectNames[ETOUI(BOSSTOMB_SKILL::BLUST_END)])
		{
			CGameInstance::Get().StopEffect(m_iCurEffectID);
		}
		
	}
	Active_Skill();
	Active_Dynamic_Effect();
}

void CBossTMB::FixedUpdate(E::_float fTimeDelta)
{
	if (!m_bDonMove)
		m_pCharacterMotor->FixedUpdate(fTimeDelta);
	/*----------- 광윤 추가 -----------*/
	auto AdditionalLight = CGameInstance::Get().GetGameObjectByHandleT<CLight>(AdditionalLightHandle.value());
	if (nullptr == AdditionalLight) return;
	
	XMVECTOR LookVec = GetTransform().GetState(STATE::LOOK);
	XMVECTOR PosVec = GetTransform().GetState(STATE::POSITION);
	
	_float3	 LightOffset = { 0.f, 10.f, 0.f };
	
	AdditionalLight->Set_LightPosition(PosVec + LookVec * 12.f + XMLoadFloat3(&LightOffset));
	/*---------------------------------*/
}

void CBossTMB::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);
	Dead();
}

void CBossTMB::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);
}

void CBossTMB::Set_AttTable(ATTMON eType, _float2 fSkillRatio)
{
	if (eType == ATTMON::END)
		return;

	if (m_eLastSkillTable == eType)
		return;

	uint32_t iSkillNum = Find_SkillNum(eType);
	if (iSkillNum == UINT_MAX || iSkillNum >= ETOUI(BOSSTOMB_SKILL::END))
		return;
	
	m_CurEffectName = m_EffectNames[iSkillNum];
	m_eLastSkillTable = m_eAttType = eType;
	m_fSkillRatio = fSkillRatio;
	++m_iCurSkill;
	m_bStar = true;
	
}

_string CBossTMB::Get_SkillName(ATTMON SkillNode)
{
	auto pValue = m_MonSkillLists.find(SkillNode);

	if (pValue == m_MonSkillLists.end())
		return "";

	if (pValue->second >= ETOUI(BOSSTOMB_SKILL::END))
		return "";

	return MagicEnumToStringView(static_cast<BOSSTOMB_SKILL>(pValue->second)).data();
}

void CBossTMB::Skill_Finished()
{
	__super::Skill_Finished();
	m_bStar = false;

	if (auto pWeapon =
		CGameInstance::Get().GetGameObjectByHandleT<CBossMace>(
			m_Partes[ETOUI(PARTES::WEAPON)]))
	{
		pWeapon->Reset_Active();
	}
}

_bool CBossTMB::Check_Table(PLAYER_SKILL_TYPE eType)
{
	
	Damaged(eType);
	if (eType == PLAYER_SKILL_TYPE::ATTACK)
	{
		++m_iNormalHitCnt;
		const auto hUIController = GET_SINGLE(UIManager)->GetUIController();
	
		if (hUIController.has_value())
		{
			if (auto* pUIController = CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*hUIController))
			{
				pUIController->AddFinisher(2.f);
			}
		}
	}

	if (eType == PLAYER_SKILL_TYPE::ATTACK)
		return false;

	if (Check_Flag(ETOUI(CBTRoot::BTFLAG::SUPERARMOR)))
		return false;

	if (eType == PLAYER_SKILL_TYPE::END || eType == PLAYER_SKILL_TYPE::DEFAULT)
		return false;

	MON_HIT_INFO HitInfo{};
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::HIT), FLAGTYPE::ADD);
	HitInfo.eAttType = m_eAttType;
	HitInfo.eHitType = eType;
	m_PendingMonTable = HitInfo;
	m_bPending = true;

	return true;
}

void CBossTMB::Active_Dynamic_Effect()
{
	if (Check_Flag(ETOUI(CBTRoot::BTFLAG::ENDHIT)))
		return;

	_float fRatio = m_pModelAnimator->GetPlayAnimRatio();
	if (m_CurEffectName == "Boss_GroundCrash" && Check_Flag(ETOUI(CBTRoot::BTFLAG::EFFECT))) {

		if (m_fSkillRatio.x <= fRatio)
		{
			CBoss_StarBurst::STARBURST_DESC desc{};
			desc.fSpeed = 140.f;
			desc.pTargetHandle = m_TargetHandle;
			desc.vStartPosition = { GetTransform().GetPosition() };
			CGameInstance::Get().AddGameObjectToLayer(LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_BossStarBurst, "BossStarBurst", &desc);
			m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::EFFECT), FLAGTYPE::DEL);	
		}
	}
	else if (m_CurEffectName == "MorningStarAfterEffect" && Check_Flag(ETOUI(CBTRoot::BTFLAG::EFFECT))) {
		
		if (m_fSkillRatio.x <= fRatio)
		{
			if (auto pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CBossMace>(m_Partes[ETOUI(PARTES::WEAPON)]))
			{
				pWeapon->Active_Effect(m_CurEffectName);
				m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::EFFECT), FLAGTYPE::DEL);
				int32_t iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_RightHand");

				if (iBoneIndex >= m_pComModelInstance->Get_CombinedBoneMatrices().size())
					return;
				_float4x4 BoneMat = m_pComModelInstance->Get_CombinedBoneMatrices()[iBoneIndex];
				OverLabTest(XMLoadFloat4x4(&BoneMat).r[3], 150.f, 30);
			}
		}

	}
	if (m_CurEffectName == m_EffectNames[ETOUI(BOSSTOMB_SKILL::BALL)] && Check_Flag(ETOUI(CBTRoot::BTFLAG::EFFECT))) {
		CMonEffectBall::MON_BALL desc{};
		desc.fDamage = 50.f;
		desc.hTarget = m_TargetHandle;
		desc.hOwner = GetHandle();
		desc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_RightHand");
		CGameInstance::Get().AddGameObjectToLayer(LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_BossBall, m_EffectNames[ETOUI(BOSSTOMB_SKILL::BALL)], &desc);
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::EFFECT), FLAGTYPE::DEL);

	}
}

void CBossTMB::Dead()
{
	if (Check_Flag(ETOUI(CBTRoot::BTFLAG::DEAD)) && m_CurEffectName == m_EffectNames[ETOUI(BOSSTOMB_SKILL::DEAD)])
	{
		if (m_pModelAnimator->GetFinish())
		{
			SetPendingDestroyCascade();
		}
	}
}

E::UPtr<CBossTMB> CBossTMB::Create()
{
	auto pInstance = E::ToUPtr(new CBossTMB{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBossTMB");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CBossTMB::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBossTMB{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBossTMB");
		return nullptr;
	}

	return pInstance;
}
