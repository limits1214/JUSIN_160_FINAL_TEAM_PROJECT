#include "pch.h"
#include "EnderDragon.h"
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
#include "DbgLineRender.h"
#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "UIController.h"
#include "UIManager.h"
//FSM
#include "EnderDragon_State.h"
#include "Edg_Spawn.h"
#include "Edg_Combat.h"
#include "Edg_Hit.h"
#include "Edg_Phase.h"
//BB
#include "BlackBoardKey.h"
#include "BTBlackBoard.h"
//Skill
#include "EdgFireBall.h"
#include "EdgBreath.h"
#include "EdgPulse.h"
NS_USING(Client)

CEnderDragon::CEnderDragon()
{
}


CEnderDragon::~CEnderDragon()
{
}

void CEnderDragon::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::DragInt("HP", &m_iHp, 0, 1);
	
	if (ImGui::Button("AddWay"))
	{
		auto pSrc = CGameInstance::Get().GetActiveCamera();
		if (nullptr == pSrc) return;

		m_DebugPoint.push_back(pSrc->GetTransform().GetPosition());
	}
	if (ImGui::Button("DelWay"))
	{
		if (!m_DebugPoint.empty())
			m_DebugPoint.pop_back();
	}
	if (ImGui::Button("SaveWay"))
		m_bPopup = true;

	if (m_bPopup)
	{
		_string Path = "./Resources/json/WayPoint/";

		ImGui::OpenPopup("SaveWay");
		_char NameBUffer[64]{};
		if (ImGui::BeginPopup("SaveWay", ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("FileName");
			if (ImGui::InputText("##FileName", &NameBUffer[0], IM_ARRAYSIZE(NameBUffer))) //이름 입력
			{
				m_WayName = NameBUffer;

			}
			if (ImGui::Button("Ok"))
			{
				m_bPopup = false;
				if (m_WayName.empty())
				{
					ImGui::CloseCurrentPopup();
					MSG_BOX("NoName");
				}
				else
				{
					nlohmann::json j;
					JsonSaveLoadManager::SaveJsonTypeFloat3list(j, m_WayName, m_DebugPoint);
					Path += m_WayName + ".json";
					std::ofstream path(Path);
					path << j.dump(4);
					path.close();
				}
			} ImGui::SameLine(100.f);
			if (ImGui::Button("Cancle"))
			{
				m_bPopup = false;
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}
	}
	Phase_Debug();
}

HRESULT CEnderDragon::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	return S_OK;
}

HRESULT CEnderDragon::Initialize(void* pArg)
{
	auto MonDesc = static_cast<DRAGON_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}
	m_iHp = m_iMaxHp = 555800;
	
	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody, "ComPxRigidBody", &Desc, &m_pComRigidBody)))
		{
			MSG_BOX("Create Failed ComPxRigidBody EnderDragon");
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
			MSG_BOX("Create Failed ComPxSphereCollider EnderDragon");
			return E_FAIL;
		}
		if (!m_pComSphereCol->SetQueryEnabled(false))
			return E_FAIL;
	}

	//피직스
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
	//캐릭컨트롤러
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
	//캐릭 모터
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
	
	if (FAILED(Ready_Fsm(MonDesc->LevelTag)))
	{
		MSG_BOX("Create Failed Fsm");
		return E_FAIL;
	}
	if (FAILED(Ready_Skill(MonDesc->LevelTag)))
	{
		MSG_BOX("Create Failed Skill");
		return E_FAIL;
	}
	Ready_BBKeyValue();

	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();
	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pModelAnimator->Build_BoneMatrices_CPU(0.f);
	GetTransform().SetPosition(XMLoadFloat3(&MonDesc->vPos));
	return S_OK;
}
HRESULT CEnderDragon::Ready_Fsm(const _string& LevelTag)
{
	CEnderDragon_State::DESC Desc{};
	if (FAILED(AddComponentFromProto(LevelTag, "Prototype_Component_Dragon_FSM", "EnderDragon_Fsm", &Desc, &m_pFsm))) return E_FAIL;


	if (false == m_pFsm->Add_State(EDG_STATE::SPAWN, CEdg_Spawn::Create())) return E_FAIL;

	if (false == m_pFsm->Add_State(EDG_STATE::COMBAT, CEdg_Combat::Create())) return E_FAIL;

	if (false == m_pFsm->Add_State(EDG_STATE::HIT, CEdg_Hit::Create())) return E_FAIL;

	if (false == m_pFsm->Add_State(EDG_STATE::PHASE_CHANGE, CEdg_Phase::Create())) return E_FAIL;

	if (false == m_pFsm->Initialize_State(EDG_STATE::SPAWN)) return E_FAIL;


	return S_OK;
}
HRESULT CEnderDragon::Ready_Skill(const _string& LevelTag)
{
	if (ETOUI(DRAGON_SKILL::END) > ETOUI(ATTMON::END))
		return E_FAIL;

	m_MonSkillLists[ATTMON::SLOT0] = ETOUI(DRAGON_SKILL::BOOM);
	m_MonSkillLists[ATTMON::SLOT1] = ETOUI(DRAGON_SKILL::BREATH);
	m_MonSkillLists[ATTMON::SLOT2] = ETOUI(DRAGON_SKILL::FIREBALL);
	m_MonSkillLists[ATTMON::SLOT3] = ETOUI(DRAGON_SKILL::PULSE);


	//////////////////////파티클 넣는곳/////////////////////////
	m_EffectNames[ETOUI(DRAGON_SKILL::FIREBALL)] = "FireBall";
	m_EffectNames[ETOUI(DRAGON_SKILL::BREATH)] = "DragonBreath";
	m_EffectNames[ETOUI(DRAGON_SKILL::PULSE)] = "Pulse";
	////////////////////////////////////////////////////////////
	CDragonSkill::EDG_SKILL_DESC SkillDesc{};
	int32_t iFireBall{};
	
	iFireBall = SkillDesc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_Head");
	
	SkillDesc.hOwner = GetHandle();
	
	auto BreathHandle = CGameInstance::Get().AddGameObjectToLayer(LevelTag, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_Breath, "03.Breath", &SkillDesc);
	if (!BreathHandle) return E_FAIL;

	///Pulse///
	SkillDesc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("chest_Main");
	auto PulseHandle = CGameInstance::Get().AddGameObjectToLayer(LevelTag, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_Pulse, "03.Pulse", &SkillDesc);
	if (!PulseHandle) return E_FAIL;
	///////////

	int32_t iOffsetBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_Mouth");
	m_SkillHandle[ETOUI(DRAGON_SKILL::FIREBALL)] = EDG_SKILL_INFO{ .bPool = false, .iBoneIndex = iFireBall,
	.LevelTag = LevelTag, .ProtoTag = PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_FireBall, .NameTag = "03.FireBall",.iOffsetBoneIndex = iOffsetBoneIndex };

	m_SkillHandle[ETOUI(DRAGON_SKILL::BREATH)] = EDG_SKILL_INFO{.handle = BreathHandle.value(), .bPool = true,};
	m_SkillHandle[ETOUI(DRAGON_SKILL::PULSE)]  = EDG_SKILL_INFO{.handle = PulseHandle.value() , .bPool = true,};

	return S_OK;
}
void CEnderDragon::Ready_BBKeyValue()
{
	auto pBB = Get_BlackBoard();
	pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, m_ePhase);
	pBB->Set_Value<MOVE>(EDG_KEY::EPATROL, MOVE::LEFT);

}
void CEnderDragon::PriorityUpdate(E::_float fTimeDelta)
{
	//if (CGameInstance::Get().KeyPressing(DIK_LCONTROL) && CGameInstance::Get().KeyDown(DIK_L))
	//	m_bDebug = !m_bDebug;
	//
	//if (!m_bDebug) return;



	Check_Phase();
	m_pFsm->PriorityUpdate(fTimeDelta);
	Update_BBToFsm();
	__super::PriorityUpdate(fTimeDelta);
	m_pFsm->Update(fTimeDelta);
}

void CEnderDragon::Update(E::_float fTimeDelta)
{
	//if (!m_bDebug) return;
	__super::Update(fTimeDelta);
	//m_pFsm->Update(fTimeDelta);
}

void CEnderDragon::FixedUpdate(E::_float fTimeDelta)
{
	//if (!m_bDebug) return;
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
}
void CEnderDragon::LateUpdate(E::_float fTimeDelta)
{
	//if (!m_bDebug) return;
	m_pFsm->LateUpdate(fTimeDelta);
	__super::LateUpdate(fTimeDelta);

}


void CEnderDragon::Set_StateFinished(_bool bFinished)
{
	//스테이트가 완료된 판정에 대해서 다시 초기회
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return;

	pBB->Set_Value(EDG_KEY::BSTATE_FINISHED, bFinished);
}
_bool CEnderDragon::Is_StateFinished()
{
	//스테이트가 끝났는지 확인
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return false;

	auto pbFinished = pBB->Get_Value<_bool>(EDG_KEY::BSTATE_FINISHED);
	if (nullptr == pbFinished) return false;
	
	return *pbFinished;
}
_string CEnderDragon::Get_SkillName(ATTMON SkillNode)
{
	auto pValue = m_MonSkillLists.find(SkillNode);

	if (pValue == m_MonSkillLists.end())
		return "";

	if (pValue->second >= ETOUI(DRAGON_SKILL::END))
		return "";

	return MagicEnumToStringView(static_cast<DRAGON_SKILL>(pValue->second)).data();
}

_bool CEnderDragon::Check_Table(PLAYER_SKILL_TYPE eType)
{
	if (eType == PLAYER_SKILL_TYPE::END || eType == PLAYER_SKILL_TYPE::DEFAULT)
		return false;

	Damaged(eType);
	if (eType == PLAYER_SKILL_TYPE::ATTACK)
	{
		const auto hUIController = GET_SINGLE(UIManager)->GetUIController();
		if (hUIController.has_value())
		{
			if (auto* pUIController = CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*hUIController))
			{
				pUIController->AddFinisher(2.f);
			}
		}
	}

	if (true == BreakSkillType(eType) && false == m_bIsBreak)
	{
		m_pFsm->Request_State(EDG_STATE::HIT);
		m_bIsBreak = true;

		m_PendingMonTable.eAttType = m_eAttType;
		m_PendingMonTable.eHitType = eType;
	
		m_bPending = true;
	}
	
	return true;

}
void CEnderDragon::Check_Phase()
{
	if (m_pFsm->GetCurState() != EDG_STATE::COMBAT)
		return;
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return;

	auto pSrc = Get_Target();
	if (nullptr == pSrc) return;

	_float3 DestPos = pSrc->GetTransform().GetPosition();
	_float3 SrcPos = GetTransform().GetPosition();

	_float fDis = XMVectorGetX(XMVector3Length(XMLoadFloat3(&DestPos) - XMLoadFloat3(&SrcPos)));

	auto pFinished = pBB->Get_Value<_bool>(EDG_KEY::BSTATE_FINISHED);
	if (nullptr == pFinished) return;

	//if (false == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE1)] && *pFinished)
	//{
	//	
	//	m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE1)] = true;
	//	m_pFsm->Request_State(EDG_STATE::PHASE_CHANGE);
	//
	//	pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE2);
	//	return;
	//}
	 
	if (false == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE2)] && m_iHp <= m_iMaxHp - m_iMaxHp / 8.f)
	{
		//피 조금 까이고 도망
		m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE2)] = true;
		m_pFsm->Request_State(EDG_STATE::PHASE_CHANGE);

		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE,DRAGON_PHASE::PHASE2);
		return;
	}
	else if (false == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE3)] && fDis <= 10.f)
	{
		//도망간 후 파이어볼 잠깐 쏘다 거리 가까워지면 다시 run
		m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE3)] = true;
		m_pFsm->Request_State(EDG_STATE::PHASE_CHANGE);

		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE3);
		return;
	}
	else if (false == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE4)] && m_iHp <= m_iMaxHp / 2.f)
	{
		//대충 날다 두드려 맞고 도망
		m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE4)] = true;
		m_pFsm->Request_State(EDG_STATE::PHASE_CHANGE);

		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE5);
		return;
	}
	else if (false == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE4)] && m_iHp <= m_iMaxHp / 3.f)
	{
		//대충 땅바닥 진입전 마지막 비행
		m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE5)] = true;
		m_pFsm->Request_State(EDG_STATE::PHASE_CHANGE);

		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE5);
		return;
	}
	else if (false == m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE6)] && m_iHp <= m_iMaxHp / 4.f)
	{
		//바닥 마지막전투
		//죽어
		m_bPhaseLock[ETOUI(DRAGON_PHASE::PHASE6)] = true;
		m_pFsm->Request_State(EDG_STATE::PHASE_CHANGE);

		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE6);
		return;
	}
}
void CEnderDragon::Set_AttTable(ATTMON eType, _float2 fSkillRatio)
{
	if (eType == ATTMON::END)
		return;

	uint32_t iSkillNum = Find_SkillNum(eType);
	if (iSkillNum == UINT_MAX || iSkillNum >= ETOUI(DRAGON_SKILL::END))
		return;

	if (true == m_SkillHandle[iSkillNum].bPool)
	{
		auto pSkill = CGameInstance::Get().GetGameObjectByHandleT<CDragonSkill>(m_SkillHandle[iSkillNum].handle);
		if (nullptr == pSkill)
			return;
		pSkill->Active(m_EffectNames[iSkillNum]);
	}
	else
	{
		CDragonSkill::EDG_SKILL_DESC SkillDesc{};
		SkillDesc.hOwner = GetHandle();
		SkillDesc.iBoneIndex = m_SkillHandle[iSkillNum].iBoneIndex;
		SkillDesc.iOffsetBoneIndex = m_SkillHandle[iSkillNum].iOffsetBoneIndex;
		auto SkillHandle = CGameInstance::Get().AddGameObjectToLayer(m_SkillHandle[iSkillNum].LevelTag, 
			m_SkillHandle[iSkillNum].ProtoTag, m_SkillHandle[iSkillNum].NameTag, &SkillDesc);
		if (!SkillHandle) return;

		auto pSkill = CGameInstance::Get().GetGameObjectByHandleT<CDragonSkill>(SkillHandle.value());
		if (nullptr == pSkill)
			return;
		pSkill->Active(m_EffectNames[iSkillNum]);
	}
	m_CurEffectName.clear();
	m_eAttType = ATTMON::END;
	m_eLastSkillTable = m_eAttType = eType;

}
void CEnderDragon::Flag_Check(_float fTimeDelta)
{
	//드래곤은 개별로 제어해서 체크해야겠네
	//사실 필요 없을지도~
	//플래그 왜만든거지
	//아아..
}
void CEnderDragon::Update_BBToFsm()
{
	auto pBB = Get_BlackBoard();

	if (nullptr == pBB)
		return;

	pBB->Set_Value(EDG_KEY::STATE, m_pFsm->GetCurState());
}
_bool CEnderDragon::BreakSkillType(PLAYER_SKILL_TYPE eType)
{
	uint32_t iSkillNumber = Find_SkillNum(m_eAttType);
	//파훼 됨?
	switch (eType)
	{
	case PLAYER_SKILL_TYPE::ACCIO:
		if (ETOUI(DRAGON_SKILL::FIREBALL) == iSkillNumber)
			return true;
		break;
	case PLAYER_SKILL_TYPE::DEPULSO:
		break;

	case PLAYER_SKILL_TYPE::DESCENDO:
		break;

	case PLAYER_SKILL_TYPE::ACIENT_LIGHTNING:
		break;
	}
	return false;
}
void CEnderDragon::Phase_Debug()
{
	uint32_t i = 0;
	for (auto& iter : m_DebugPoint)
	{
		auto pDbgLineRender = CGameInstance::Get().GetDbgLineRender();

		const auto vPreviousColor = pDbgLineRender->GetColor();
		const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
		pDbgLineRender->SetColor({ 0.f, 1.f, 1.f, 1.f });
		pDbgLineRender->SetDepthTest(true);
		pDbgLineRender->AddSphere(1.2f, XMMatrixTranslation(iter.x, iter.y, iter.z));
		pDbgLineRender->SetColor(vPreviousColor);
		pDbgLineRender->SetDepthMode(ePreviousDepthMode);
		Picking(iter, 0x44524750 + i++);
	}
	auto pBB = Get_BlackBoard();
	if (nullptr == pBB) return;
	if (CGameInstance::Get().KeyPressing(DIK_LCONTROL) && CGameInstance::Get().KeyDown(DIK_Q))
	{
		m_pFsm->Request_State(EDG_STATE::PHASE_CHANGE);
		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE1);

	}
	else if (CGameInstance::Get().KeyPressing(DIK_LCONTROL) && CGameInstance::Get().KeyDown(DIK_W))
	{
		m_pFsm->Request_State(EDG_STATE::PHASE_CHANGE);
		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE2);
	}
	else if (CGameInstance::Get().KeyPressing(DIK_LCONTROL) && CGameInstance::Get().KeyDown(DIK_E))
	{
		m_pFsm->Request_State(EDG_STATE::PHASE_CHANGE);
		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE3);
	}
	else if (CGameInstance::Get().KeyPressing(DIK_LCONTROL) && CGameInstance::Get().KeyDown(DIK_R))
	{
		m_pFsm->Request_State(EDG_STATE::PHASE_CHANGE);
		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE4);
	}
	else if (CGameInstance::Get().KeyPressing(DIK_LCONTROL) && CGameInstance::Get().KeyDown(DIK_T))
	{
		m_pFsm->Request_State(EDG_STATE::PHASE_CHANGE);
		pBB->Set_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE, DRAGON_PHASE::PHASE4);
	}
}
void CEnderDragon::Picking(_float3& vPos, uint32_t iID)
{
	auto pCamera =CGameInstance::Get().GetActiveCamera();

	ImGuiViewport* pViewport =ImGui::GetMainViewport();

	if (!pCamera || !pViewport)
		return;

	_float4x4 View{};
	_float4x4 Projection{};
	_float4x4 World{};

	XMStoreFloat4x4(&View,	pCamera->GetView());
	XMStoreFloat4x4(&Projection,pCamera->GetProj());

	XMStoreFloat4x4(&World,XMMatrixTranslation(	vPos.x,	vPos.y,vPos.z));

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList(pViewport));

	ImGuizmo::SetRect(pViewport->Pos.x,pViewport->Pos.y,pViewport->Size.x,pViewport->Size.y);;
	ImGuizmo::SetID(iID);
	if (!ImGuizmo::Manipulate(	&View._11,&Projection._11,ImGuizmo::TRANSLATE,ImGuizmo::WORLD,&World._11))
		return;
	
	vPos ={ World._41, World._42, World._43};

	return ;
}
E::UPtr<CEnderDragon> CEnderDragon::Create()
{
	auto pInstance = E::ToUPtr(new CEnderDragon{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CEnderDragon");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CEnderDragon::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CEnderDragon{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CEnderDragon");
		return nullptr;
	}

	return pInstance;
}
