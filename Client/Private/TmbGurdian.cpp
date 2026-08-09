#include "pch.h"
#include "TmbGurdian.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "Mon_Weapon.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "ComPxCharacterController.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "DbgLineRender.h"
#include "TmbGurdianDead.h"
#include "GurdianWeapon.h"
#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "UIController.h"
#include "UIManager.h"
#include "BTBlackBoard.h"
NS_USING(Client)

namespace
{
	constexpr std::array<const char*, 21> TMB_DEBRIS_MAJOR_BONES{
		"Hips",
		"Spine",
		"Spine1",
		"Spine2",
		"Spine3",
		"Neck",
		"face",
		"LeftShoulder",
		"LeftArm",
		"LeftForeArm",
		"LeftHand",
		"RightShoulder",
		"RightArm",
		"RightForeArm",
		"RightHand",
		"LeftUpLeg",
		"LeftLeg",
		"LeftFoot",
		"RightUpLeg",
		"RightLeg",
		"RightFoot"
	};

	struct TMB_BONE_BIND_DATA
	{
		int32_t iBoneIndex{ -1 };
		_float4x4 matInverseBind{};
		_float3 vBindPosition{};
	};

	std::vector<TMB_BONE_BIND_DATA>
		BuildMajorBoneBindData(
			const SPtr<CResModel>& pModel)
	{
		std::vector<TMB_BONE_BIND_DATA> vecResult{};
		if (!pModel)
			return vecResult;

		for (const char* pBoneName :
			TMB_DEBRIS_MAJOR_BONES)
		{
			const int32_t iBoneIndex =
				pModel->Get_BoneIndex(pBoneName);
			if (iBoneIndex < 0)
				continue;

			for (const auto& pMesh :
				pModel->GetMeshes())
			{
				if (!pMesh)
					continue;

				const auto& vecBoneIndices =
					pMesh->GetBoneIndices();
				const auto& vecOffsetMatrices =
					pMesh->GetOffsetMatrices();
				const size_t iCount = std::min(
					vecBoneIndices.size(),
					vecOffsetMatrices.size());

				for (size_t i = 0; i < iCount; ++i)
				{
					if (vecBoneIndices[i] !=
						static_cast<uint32_t>(
							iBoneIndex))
					{
						continue;
					}

					_vector vDeterminant{};
					const _matrix matBind =
						XMMatrixInverse(
							&vDeterminant,
							XMLoadFloat4x4(
								&vecOffsetMatrices[i]));
					const _float fDeterminant =
						XMVectorGetX(vDeterminant);
					if (!std::isfinite(fDeterminant) ||
						std::abs(fDeterminant) <=
						FLT_EPSILON)
					{
						continue;
					}

					TMB_BONE_BIND_DATA tData{};
					tData.iBoneIndex = iBoneIndex;
					tData.matInverseBind =
						vecOffsetMatrices[i];
					XMStoreFloat3(
						&tData.vBindPosition,
						matBind.r[3]);
					vecResult.push_back(tData);
					break;
				}

				if (!vecResult.empty() &&
					vecResult.back().iBoneIndex ==
					iBoneIndex)
				{
					break;
				}
			}
		}

		return vecResult;
	}

	const TMB_BONE_BIND_DATA*
		FindClosestBone(
			const std::vector<TMB_BONE_BIND_DATA>&
				vecBoneBindData,
			const _float3& vDebrisCenter)
	{
		const TMB_BONE_BIND_DATA* pClosest{};
		_float fClosestDistanceSq = FLT_MAX;

		for (const auto& tBone : vecBoneBindData)
		{
			const _vector vDelta =
				XMLoadFloat3(&vDebrisCenter) -
				XMLoadFloat3(&tBone.vBindPosition);
			const _float fDistanceSq =
				XMVectorGetX(
					XMVector3LengthSq(vDelta));
			if (fDistanceSq >= fClosestDistanceSq)
				continue;

			fClosestDistanceSq = fDistanceSq;
			pClosest = &tBone;
		}

		return pClosest;
	}

	_bool ResolveApproximateDebrisBoneBinding(
		const SPtr<CResStaticModel>& pDebrisModel,
		const std::vector<TMB_BONE_BIND_DATA>&
			vecBoneBindData,
		int32_t& iOutBoneIndex,
		_float4x4& matOutInverseBind)
	{
		if (!pDebrisModel ||
			!pDebrisModel->HasLocalBounds())
		{
			return false;
		}

		const auto* pClosestBone = FindClosestBone(
			vecBoneBindData,
			pDebrisModel->GetLocalBounds().Center);
		if (!pClosestBone)
			return false;

		iOutBoneIndex = pClosestBone->iBoneIndex;
		matOutInverseBind =
			pClosestBone->matInverseBind;
		return true;
	}

	_bool ResolveDebrisBoneBinding(
		uint32_t iDebrisIndex,
		const SPtr<CResStaticModel>& pDebrisModel,
		const std::vector<TMB_BONE_BIND_DATA>&
			vecBoneBindData,
		int32_t& iOutBoneIndex,
		_float4x4& matOutInverseBind)
	{
		// This is the single replacement point for an explicit
		// debris-index-to-bone mapping table or metadata file.
		(void)iDebrisIndex;
		return ResolveApproximateDebrisBoneBinding(
			pDebrisModel,
			vecBoneBindData,
			iOutBoneIndex,
			matOutInverseBind);
	}
}

CTmbGurdian::CTmbGurdian()
{
}

CTmbGurdian::~CTmbGurdian()
{
}

_bool CTmbGurdian::UpdateDeadDebrisPoseFromCurrentBones()
{
	if (!m_pComModelInstance)
		return false;

	const auto& vecCombinedBoneMatrices =
		m_pComModelInstance
			->Get_CombinedBoneMatrices();
	const size_t iPoseCount = std::min(
		m_vecDeadHandles.size(),
		std::min(
			m_vecDeadBoneIndices.size(),
			m_vecDeadInverseBindMatrices.size()));
	if (iPoseCount == 0)
		return false;

	_bool bAllUpdated =
		iPoseCount == m_vecDeadHandles.size();
	for (size_t i = 0; i < iPoseCount; ++i)
	{
		const int32_t iBoneIndex =
			m_vecDeadBoneIndices[i];
		if (iBoneIndex < 0 ||
			static_cast<size_t>(iBoneIndex) >=
			vecCombinedBoneMatrices.size())
		{
			bAllUpdated = false;
			continue;
		}

		auto* pDebris = CGameInstance::Get()
			.GetGameObjectByHandleT<
				CTmbGurdianDead>(
					m_vecDeadHandles[i]);
		if (!pDebris)
		{
			bAllUpdated = false;
			continue;
		}

		const _matrix matBoneWorld =
			XMLoadFloat4x4(
				&vecCombinedBoneMatrices[
					iBoneIndex]) *
			GetTransform().GetLoadedWorldMatrix();
		if (!pDebris->ApplyBonePose(
				matBoneWorld,
				XMLoadFloat4x4(
					&m_vecDeadInverseBindMatrices[i])))
		{
			bAllUpdated = false;
		}
	}

	return bAllUpdated;
}

_bool CTmbGurdian::ActivateDeadDebrisPhysics()
{
	if (m_bDeadDebrisPhysicsActivated)
		return true;

	if (!UpdateDeadDebrisPoseFromCurrentBones())
	{
		m_bDeadDebrisPhysicsActivated = false;
		return false;
	}

	_bool bAllActivated =
		!m_vecDeadHandles.empty();

	for (const CHandle& hDebris :
		m_vecDeadHandles)
	{
		auto* pDebris = CGameInstance::Get()
			.GetGameObjectByHandleT<
				CTmbGurdianDead>(hDebris);
		if (!pDebris)
		{
			bAllActivated = false;
			continue;
		}

		pDebris->SetRenderEnabled(true);
		if (!pDebris->ActivatePhysics())
			bAllActivated = false;
	}

	m_bDeadDebrisPhysicsActivated =
		bAllActivated;
	if (bAllActivated)
		m_bRenderDeadDebris = true;
	return bAllActivated;
}

void CTmbGurdian::Set_AttTable(ATTMON eType, _float2 fSkillRatio)
{
	if (eType == ATTMON::END)
		return;

	if (m_eLastSkillTable == eType)
		return;

	uint32_t iSkillNum = Find_SkillNum(eType);
	if (iSkillNum == UINT_MAX || iSkillNum >= ETOUI(TOMB_SKILL::END))
		return;

	m_CurEffectName = m_EffectNames[iSkillNum];
	m_eLastSkillTable = m_eAttType = eType;
	m_fSkillRatio = fSkillRatio;
	++m_iCurSkill;
}

_string CTmbGurdian::Get_SkillName(ATTMON SkillNode)
{
	auto pValue = m_MonSkillLists.find(SkillNode);
	
	if (pValue == m_MonSkillLists.end())
		return "";
	
	if (pValue->second >= ETOUI(TOMB_SKILL::END))
		return "";
	
	return MagicEnumToStringView(static_cast<TOMB_SKILL>(pValue->second)).data();
}

const _float CTmbGurdian::Get_Damage()
{
	//TOMB_SKILL::JUMP_END, TOMB_SKILL::SLASH
	uint32_t SkillID = Find_SkillNum(m_eAttType);

	if(SkillID == ETOUI(TOMB_SKILL::JUMP_END))
	{
		m_fDamage = 25.f;
	}
	else if (SkillID == ETOUI(TOMB_SKILL::SLASH))
	{
		m_fDamage = 5.f;
	}
	else if (SkillID == ETOUI(TOMB_SKILL::SMASH))
	{
		m_fDamage = 15.f;
	}
	else if (SkillID == ETOUI(TOMB_SKILL::STING))
	{
		m_fDamage = 10.f;
	}
	
	return m_fDamage;
}

_bool CTmbGurdian::Check_Table(PLAYER_SKILL_TYPE eType)
{
	if (eType == PLAYER_SKILL_TYPE::END || eType == PLAYER_SKILL_TYPE::DEFAULT)
		return false;

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

	if (m_eMonType == MONSTER_TYPE::NORMAL)
	{
		uint32_t iIndex = Find_SkillNum(m_eAttType);
		if (iIndex != ETOUI(TOMB_SKILL::HIT_ACCIO))
		{
			Skill_Finished();
		}
	}

	if (eType == PLAYER_SKILL_TYPE::ATTACK && Check_Flag(ETOUI(CBTRoot::BTFLAG::NOCKDOWN)))
		return false;

	if (ETOUI(m_eMonType) == ETOUI(MONSTER_TYPE::ELITE) && eType == PLAYER_SKILL_TYPE::ATTACK)
		return false;
	
	if(eType != PLAYER_SKILL_TYPE::ATTACK)
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::NOCKDOWN), FLAGTYPE::DEL);

	if (Check_Flag(ETOUI(CBTRoot::BTFLAG::SUPERARMOR)) && eType == PLAYER_SKILL_TYPE::ATTACK)
		return false;

	

	MON_HIT_INFO HitInfo{};
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::HIT), FLAGTYPE::ADD);

	HitInfo.eAttType = m_eAttType;
	HitInfo.eHitType = eType;
	m_PendingMonTable = HitInfo;
	m_bPending = true;

	return true;
}

void CTmbGurdian::UpdateGUI()
{
	__super::UpdateGUI();

	const char* pDebrisRenderButton =
		m_bRenderDeadDebris ?
		"Hide Dead Debris" :
		"Show Dead Debris";
	if (ImGui::Button(pDebrisRenderButton))
	{
		m_bRenderDeadDebris =
			!m_bRenderDeadDebris;

		if (m_bRenderDeadDebris)
			UpdateDeadDebrisPoseFromCurrentBones();

		for (const CHandle& hDebris :
			m_vecDeadHandles)
		{
			auto* pDebris = CGameInstance::Get()
				.GetGameObjectByHandleT<
					CTmbGurdianDead>(hDebris);
			if (pDebris)
			{
				pDebris->SetRenderEnabled(
					m_bRenderDeadDebris);
			}
		}
	}
	ImGui::SameLine();
	ImGui::Text(
		"Render: %s",
		m_bRenderDeadDebris ? "ON" : "OFF");

	if (ImGui::Button(
		"Activate Dead Debris Physics"))
	{
		ActivateDeadDebrisPhysics();
	}
	ImGui::SameLine();
	ImGui::Text(
		"Physics: %s",
		m_bDeadDebrisPhysicsActivated ?
		"ACTIVE" :
		"WAITING");

	if (!m_pComModelInstance ||
		!m_pComModelInstance->GetModel())
	{
		return;
	}

	const auto& vecBones =
		m_pComModelInstance->GetModel()->GetBones();
	if (ImGui::TreeNode("Debris Bone Mapping"))
	{
		for (size_t i = 0;
			i < m_vecDeadBoneIndices.size();
			++i)
		{
			const int32_t iBoneIndex =
				m_vecDeadBoneIndices[i];
			std::string sBoneName = "Unmapped";
			if (iBoneIndex >= 0 &&
				static_cast<size_t>(iBoneIndex) <
				vecBones.size() &&
				vecBones[iBoneIndex])
			{
				sBoneName = vecBones[iBoneIndex]
					->GetBoneName();
			}

			ImGui::Text(
				"Debris %u -> %s",
				static_cast<uint32_t>(i),
				sBoneName.c_str());
		}
		ImGui::TreePop();
	}
}

HRESULT CTmbGurdian::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	return S_OK;
}

HRESULT CTmbGurdian::Initialize(void* pArg)
{
	auto MonDesc = static_cast<TMBGURDIAN_DESC*>(pArg);
	if (FAILED(__super::Initialize(pArg)))
	{
		return E_FAIL;
	}
	{
		CComPxRigidBody::DESC Desc{};
		Desc.eType = CComPxRigidBody::TYPE::KINEMATIC;
		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::PHYSX,
			ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody, "ComPxRigidBody", &Desc, &m_pComRigidBody)))
		{
			MSG_BOX("Create Failed ComPxRigidBody TmbBoss");
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
			MSG_BOX("Create Failed ComPxSphereCollider TmbBoss");
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
	Desc.LoadPath = MonDesc->BeHaviorTag;
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
	CGurdianWeapon::DESC WeaponDesc{};
	WeaponDesc.sObjectTag = "Weapon";
	WeaponDesc.ParentHandle = GetHandle();
	WeaponDesc.iBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_RightHandSocket");
	WeaponDesc.WeaponName = MonDesc->WeaponResourceName; 
	WeaponDesc.LevelTag = MonDesc->LevelTag;
	WeaponDesc.vScale = MonDesc->vWeaponScale;
	WeaponDesc.vOwnerScale = MonDesc->vScale;
	auto Weapon = E::CGameInstance::Get().AddGameObjectToLayer(MonDesc->LevelTag, MonDesc->WeaponProtoName, "03_Weapon", &WeaponDesc);
	if (!Weapon.has_value())
	{
		MSG_BOX("Create Failed Weapon To TmbGurDian");
		return E_FAIL;
	}
	m_Partes[ETOUI(PARTES::WEAPON)] = Weapon.value();
	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();

	m_MonSkillLists[ATTMON::SLOT0] = ETOUI(TOMB_SKILL::JUMP_START);
	m_MonSkillLists[ATTMON::SLOT1] = ETOUI(TOMB_SKILL::JUMP_END);
	m_MonSkillLists[ATTMON::SLOT2] = ETOUI(TOMB_SKILL::SLASH);
	m_MonSkillLists[ATTMON::SLOT3] = ETOUI(TOMB_SKILL::SMASH);
	m_MonSkillLists[ATTMON::SLOT4] = ETOUI(TOMB_SKILL::HIT_ACCIO);
	m_MonSkillLists[ATTMON::SLOT5] = ETOUI(TOMB_SKILL::HIT_DESCENDO);
	m_MonSkillLists[ATTMON::SKIP] = ETOUI(TOMB_SKILL::SKIP);


	m_EffectNames[ETOUI(TOMB_SKILL::JUMP_START)] = "TombJumpStart";
	m_EffectNames[ETOUI(TOMB_SKILL::JUMP_END)] = "TombJumpEnd";
	m_EffectNames[ETOUI(TOMB_SKILL::HIT_ACCIO)] = "AccioGrab";
	m_EffectNames[ETOUI(TOMB_SKILL::HIT_DESCENDO)] = "Descendo_Enemy";
	m_pComTransform->SetRotation(XMVectorSet(MonDesc->vRot.x, MonDesc->vRot.y, MonDesc->vRot.z, 0.f), MonDesc->fAngle);
	m_pComTransform->SetScale(XMVectorSet(MonDesc->vScale.x, MonDesc->vScale.y, MonDesc->vScale.z, 0));
	GetTransform().Update();
	m_eAttType = ATTMON::END;

	// 죽음 파편들
	// 죽음은 바람과 같지.. 늘 내 곁에 있으니
	{
		const auto pModel = m_pComModelInstance->GetModel();
		if (!pModel)
		{
			return E_FAIL;
		}

		const _matrix matDebrisInitialWorld =
			XMLoadFloat4x4(&pModel->Get_PreTransformMatrix()) *
			GetTransform().GetLoadedWorldMatrix();

		_vector vDebrisScale{};
		_vector vDebrisRotation{};
		_vector vDebrisPosition{};
		if (!XMMatrixDecompose(
			&vDebrisScale,
			&vDebrisRotation,
			&vDebrisPosition,
			matDebrisInitialWorld))
		{
			return E_FAIL;
		}

		_float3 vInitialPosition{};
		_float4 vInitialQuaternion{};
		_float3 vInitialScale{};
		XMStoreFloat3(&vInitialPosition, vDebrisPosition);
		XMStoreFloat4(
			&vInitialQuaternion,
			XMQuaternionNormalize(vDebrisRotation));
		XMStoreFloat3(&vInitialScale, vDebrisScale);

		const auto vecBoneBindData =
			BuildMajorBoneBindData(pModel);

		for (uint32_t i = 0; i < 13; ++i)
		{
			CTmbGurdianDead::TMBGURDIAN_DEAD_DESC Desc{};
			Desc.sObjectTag = "TmbGurdianDead";
			Desc.sResourceGroup = MonDesc->LevelTag;
			Desc.DebrisResTag = "Static_Med_Debris_" + std::to_string(i);
			Desc.DebrisConvex = "./Resources/PhysX/Cooked/SM_Med_" + std::to_string(i) + ".pxconvex";
			Desc.vInitialPosition = vInitialPosition;
			Desc.vInitialQuaternion = vInitialQuaternion;
			Desc.vInitialScale = vInitialScale;
			Desc.vConvexScale = vInitialScale;
			Desc.tFilter = PX_FILTER_DESC{
				.iLayer = ETOUI(COLLISION_LAYER::DEBRIS),
				.iSimulationMask =
					ETOUI(COLLISION_LAYER::WORLD_STATIC) |
					ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
					ETOUI(COLLISION_LAYER::MOVING_PLATFORM) |
					ETOUI(COLLISION_LAYER::DEBRIS),
				.iQueryMask =
					ETOUI(COLLISION_LAYER::WORLD_STATIC) |
					ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
					ETOUI(COLLISION_LAYER::MOVING_PLATFORM)
			};
			auto debris = E::CGameInstance::Get().AddGameObjectToLayer(
				MonDesc->LevelTag,
				PROTO_GAMEOBJECT::Prototype_GameObject_TmbGurdianDead,
				"28_TmbGurdianDead",
				&Desc);
			if (!debris)
			{
				MSG_BOX("TmbGurdianDead AddLayer Failed");
				return E_FAIL;
			}
			m_vecDeadHandles.push_back(*debris);

			const auto pDebrisModel =
				CGameInstance::Get()
					.GetResourceFirst<CResStaticModel>(
						MonDesc->LevelTag,
						Desc.DebrisResTag);

			int32_t iBoneIndex{ -1 };
			_float4x4 matInverseBind{};
			XMStoreFloat4x4(
				&matInverseBind,
				XMMatrixIdentity());

			ResolveDebrisBoneBinding(
				i,
				pDebrisModel,
				vecBoneBindData,
				iBoneIndex,
				matInverseBind);
			m_vecDeadBoneIndices.push_back(iBoneIndex);
			m_vecDeadInverseBindMatrices.push_back(
				matInverseBind);
		}
	}

	m_pModelAnimator->SetEvaluationMode(CComAnimator::EVALUATION_MODE::CPU_GPU);
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DROP), FLAGTYPE::ADD);
	m_fEMissiveColor = { 1.f,1.f,1.f};
	m_eMonType = MonDesc->MonType;
	if (m_eMonType == MONSTER_TYPE::NORMAL)
		m_iHp = m_iMaxHp = 71;
	else if (m_eMonType == MONSTER_TYPE::ELITE)
		m_iHp = m_iMaxHp = 125;
	
	m_iColliderBoneIndex = m_pComModelInstance->GetModel()->Get_BoneIndex("SKT_Spine1");
	
	m_pModelAnimator->Play_Anim(0, false);
	ReadySound();
	return S_OK;
}

void CTmbGurdian::Damaged(PLAYER_SKILL_TYPE eType)
{
	__super::Damaged(eType);

	if (eType == PLAYER_SKILL_TYPE::ACIENT_LIGHTNING)
		m_iHp = 0.f;

}
void CTmbGurdian::Active_Skill()
{
	if (Check_Flag(ETOUI(CBTRoot::BTFLAG::LOOP)))
	{
		if (m_iCurEffectID != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().SetEffectWorldMatrix(m_iCurEffectID, *GetTransform().GetWorldMatrix());
		m_bSkillLoop = true;
	}
	if (m_eAttType == ATTMON::END)
		return;

	if (m_iCurSkill == m_iPreSkill)
		return;

	

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

void CTmbGurdian::ReadySound()
{
	m_SoundTable["TmbWalk"] = { "./Resources/SampleClient/Sound/PensiveKnight/Foot_Impact/Foot_Impact_Walk1.wav",
								"./Resources/SampleClient/Sound/PensiveKnight/Foot_Impact/Foot_Impact_Walk2.wav",
								"./Resources/SampleClient/Sound/PensiveKnight/Foot_Impact/Foot_Impact_Walk3.wav",
								"./Resources/SampleClient/Sound/PensiveKnight/Foot_Impact/Foot_Impact_Walk4.wav",
								"./Resources/SampleClient/Sound/PensiveKnight/Foot_Impact/Foot_Impact_Walk5.wav",
								"./Resources/SampleClient/Sound/PensiveKnight/Foot_Impact/Foot_Impact_Walk6.wav",

	};
	m_SoundTable["TmbTurn"] = { "./Resources/SampleClient/Sound/PensiveKnight/Creak_Short/Creak_Short_Turn1.wav",
								"./Resources/SampleClient/Sound/PensiveKnight/Creak_Short/Creak_Short_Turn2.wav",
								"./Resources/SampleClient/Sound/PensiveKnight/Creak_Short/Creak_Short_Turn3.wav",
								"./Resources/SampleClient/Sound/PensiveKnight/Creak_Short/Creak_Short_Turn4.wav",
								"./Resources/SampleClient/Sound/PensiveKnight/Creak_Short/Creak_Short_Turn5.wav",
								"./Resources/SampleClient/Sound/PensiveKnight/Creak_Short/Creak_Short_Turn6.wav",

	};
	m_SoundTable["TmbSlash1"] = { "./Resources/SampleClient/Sound/PensiveKnight/Sword/Slash1.wav",};
	m_SoundTable["TmbSlash2"] = { "./Resources/SampleClient/Sound/PensiveKnight/Sword/Slash2.wav", };
	m_SoundTable["TmbSlash3"] = { "./Resources/SampleClient/Sound/PensiveKnight/Sword/Slash3.wav", };
	m_SoundTable["TmbSlash4"] = { "./Resources/SampleClient/Sound/PensiveKnight/Sword/Slash4.wav", };
	m_SoundTable["TmbSlash5"] = { "./Resources/SampleClient/Sound/PensiveKnight/Sword/Slash5.wav", };
	m_SoundTable["TmbSlash6"] = { "./Resources/SampleClient/Sound/PensiveKnight/Sword/Slash6.wav", };

	m_SoundTable["TmbBeforeHit"] = { "./Resources/SampleClient/Sound/PensiveKnight/Sword/BeforeHit.wav", };
	m_SoundTable["TombEliteSpawn"] = { "./Resources/SampleClient/Sound/PensiveKnight/TombEliteSpawn.wav", };
}

void CTmbGurdian::PriorityUpdate(E::_float fTimeDelta)
{
	__super::PriorityUpdate(fTimeDelta);
}

void CTmbGurdian::FixedUpdate(E::_float fTimeDelta)
{
	m_pCharacterMotor->FixedUpdate(fTimeDelta);
	
}

void CTmbGurdian::Update(E::_float fTimeDelta)
{
	Active_Skill();
	__super::Update(fTimeDelta);

	if (m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::DEBRIS)))
	{
		// [LSY] 본체가 플래그를 지우기 전에 무기에 직접 전달해 업데이트 순서 의존성을 제거한다.
		if (auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CGurdianWeapon>(
			m_Partes[ETOUI(PARTES::WEAPON)]))
		{
			if (!pWeapon->ActivateDebrisPhysics())
				DEBUG_LOG("[TmbGurdian] Failed to activate weapon debris physics.\n");
		}

		ActivateDeadDebrisPhysics();
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DEBRIS), FLAGTYPE::DEL);

		SetPendingDestroy();
	}
}

void CTmbGurdian::LateUpdate(E::_float fTimeDelta)
{
	if (m_bDeadDebrisPhysicsActivated)
		return;

	__super::LateUpdate(fTimeDelta);
}

E::UPtr<CTmbGurdian> CTmbGurdian::Create()
{
	auto pInstance = E::ToUPtr(new CTmbGurdian{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTmbGurdian");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTmbGurdian::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTmbGurdian{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTmbGurdian");
		return nullptr;
	}

	return pInstance;
}
