#include "pch.h"
#include "NvClothCape.h"

#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComNvCloth.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "Player.h"
#include "ResModel.h"
#include "ResModelBone.h"
#include "ResNvClothMesh.h"
#include "Resources.h"

NS_USING(Client)

namespace Client::NvClothCapeDetail
{
	_bool MakeRigidMatrix(
		_fmatrix Matrix,
		_matrix& OutRigidMatrix)
	{
		_vector vScale{};
		_vector qRotation{};
		_vector vTranslation{};
		if (!XMMatrixDecompose(
			&vScale,
			&qRotation,
			&vTranslation,
			Matrix))
		{
			return false;
		}

		qRotation = XMQuaternionNormalize(qRotation);
		OutRigidMatrix =
			XMMatrixRotationQuaternion(qRotation) *
			XMMatrixTranslationFromVector(vTranslation);
		return true;
	}

	_matrix MakePoseMatrix(
		const _float3& vPosition,
		const _float4& vRotation)
	{
		return
			XMMatrixRotationQuaternion(
				XMQuaternionNormalize(
					XMLoadFloat4(&vRotation))) *
			XMMatrixTranslation(
				vPosition.x,
				vPosition.y,
				vPosition.z);
	}
}

CNvClothCape::CNvClothCape()
	: CGameObject{}
{
	XMStoreFloat4x4(
		&m_ParentWorld,
		XMMatrixIdentity());
}

CNvClothCape::CNvClothCape(
	const CNvClothCape& Prototype)
	: CGameObject{ Prototype },
	  m_pVertexShader{ Prototype.m_pVertexShader },
	  m_pShadowVertexShader{
		  Prototype.m_pShadowVertexShader },
	  m_pPointShadowVertexShader{
		  Prototype.m_pPointShadowVertexShader },
	  m_pPixelShader{ Prototype.m_pPixelShader }
{
	XMStoreFloat4x4(
		&m_ParentWorld,
		XMMatrixIdentity());
}

CNvClothCape::~CNvClothCape()
{
}

HRESULT CNvClothCape::InitializePrototype(void*)
{
	m_pVertexShader =
		CGameInstance::Get().
		GetResourceFirst<CResVertexShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"VS_NvCloth");
	m_pPixelShader =
		CGameInstance::Get().
		GetResourceFirst<CResPixelShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"PS_NvCloth");
	m_pShadowVertexShader =
		CGameInstance::Get().
		GetResourceFirst<CResVertexShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"VS_NvClothShadow");
	m_pPointShadowVertexShader =
		CGameInstance::Get().
		GetResourceFirst<CResVertexShader>(
			TAG_RES_GRP_PERMANENT_SHADER,
			"VS_NvClothPointShadow");
	return m_pVertexShader &&
		m_pShadowVertexShader &&
		m_pPointShadowVertexShader &&
		m_pPixelShader ?
		S_OK : E_FAIL;
}

HRESULT CNvClothCape::Initialize(void* pArg)
{
	const auto* pDesc = static_cast<const DESC*>(pArg);
	if (!pDesc ||
		pDesc->sResourceGroup.hash == 0 ||
		pDesc->sModelResourceTag.hash == 0 ||
		pDesc->sClothMeshResourceTag.hash == 0 ||
		pDesc->sTargetModelComponentTag.hash == 0 ||
		pDesc->sAttachBoneName.empty() ||
		!std::isfinite(pDesc->fTeleportDistance) ||
		pDesc->fTeleportDistance < 0.f ||
		!std::isfinite(pDesc->fBackstopRadius) ||
		pDesc->fBackstopRadius <= 0.f ||
		!std::isfinite(pDesc->fBackstopOffset) ||
		!std::isfinite(
			pDesc->fBackstopFullRatio) ||
		pDesc->fBackstopFullRatio < 0.f ||
		pDesc->fBackstopFullRatio > 1.f ||
		!std::isfinite(
			pDesc->fBackstopFadeEndRatio) ||
		pDesc->fBackstopFadeEndRatio < 0.f ||
		pDesc->fBackstopFadeEndRatio > 1.f ||
		pDesc->fBackstopFullRatio >=
			pDesc->fBackstopFadeEndRatio ||
		!std::isfinite(
			pDesc->fBackstopFadeDepth) ||
		pDesc->fBackstopFadeDepth < 0.f ||
		(!pDesc->tBodyCollisionRig.Shapes.empty() &&
			pDesc->tBodyCollisionRig.iVersion !=
				NVCLOTH_COLLISION_RIG_VERSION) ||
		FAILED(CGameObject::Initialize(pArg)))
	{
		return E_INVALIDARG;
	}

	m_hTarget = pDesc->hTarget;
	m_sTargetModelComponentTag =
		pDesc->sTargetModelComponentTag;
	m_sAttachBoneName =
		pDesc->sAttachBoneName;
	m_fTeleportDistance =
		pDesc->fTeleportDistance;
	m_bUseBackstop =
		pDesc->bUseBackstop;
	m_bFlipBackstopNormal =
		pDesc->bFlipBackstopNormal;
	m_fBackstopRadius =
		pDesc->fBackstopRadius;
	m_fBackstopOffset =
		pDesc->fBackstopOffset;
	m_fBackstopFullRatio =
		pDesc->fBackstopFullRatio;
	m_fBackstopFadeEndRatio =
		pDesc->fBackstopFadeEndRatio;
	m_fBackstopFadeDepth =
		pDesc->fBackstopFadeDepth;
	m_bUseVirtualParticles =
		pDesc->bUseVirtualParticles;
	m_BodyCollisionRig =
		pDesc->tBodyCollisionRig;
	GetTransform().SetPosition(
		pDesc->vLocalPosition);

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = {
			TAG_RES_GRP_PERMANENT_BUFFER,
			TAG_RES_CBUFFER_OBJECT
		};
		if (FAILED(AddComponentFromProto(
			"PERMANENT",
			"Prototype_Component_ConstantBuffer",
			"ComCBufferPerObject",
			&Desc,
			&m_pComCBufferPerObject)))
		{
			return E_FAIL;
		}
	}

	{
		CComModelInstance::DESC Desc{};
		Desc.sGroupTag = pDesc->sResourceGroup;
		Desc.sResTag = pDesc->sModelResourceTag;
		if (FAILED(AddComponentFromProto(
			"PERMANENT",
			"Prototype_Component_ModelInstance",
			"ComModelInstance",
			&Desc,
			&m_pComModelInstance)))
		{
			return E_FAIL;
		}
	}

	m_pClothMesh =
		CGameInstance::Get().
		GetResourceFirst<CResNvClothMesh>(
			pDesc->sResourceGroup,
			pDesc->sClothMeshResourceTag);
	if (!m_pClothMesh ||
		m_pClothMesh->GetSections().empty())
	{
		return E_FAIL;
	}

	{
		CComNvCloth::DESC Desc{};
		Desc.tFabric =
			m_pClothMesh->GetFabricDesc();
		Desc.tCloth.vGravity =
			{ 0.f, -9.81f, 0.f };
		Desc.tCloth.vDamping =
			{ 0.08f, 0.08f, 0.08f };
		Desc.tCloth.fSolverFrequency = 60.f;
		Desc.tCloth.fStiffnessFrequency = 60.f;
		Desc.tCloth.fPhaseStiffness = 0.9f;
		Desc.tCloth.fStretchLimit = 1.05f;
		Desc.tCloth.bUseVirtualParticles =
			m_bUseVirtualParticles;
		Desc.tCloth.vLinearInertia =
			{ 1.f, 1.f, 1.f };
		Desc.tCloth.vAngularInertia =
			{ 0.8f, 0.8f, 0.8f };
		Desc.tCloth.vCentrifugalInertia =
			{ 0.8f, 0.8f, 0.8f };
		if (FAILED(AddComponentFromProto(
			"PHYSX",
			"Prototype_Component_ComNvCloth",
			"ComNvCloth",
			&Desc,
			&m_pComNvCloth)))
		{
			return E_FAIL;
		}
	}

	if (!ResolveAttachment() ||
		!UpdateAttachment(true) ||
		!UpdateBodyCollisions())
	{
		return E_FAIL;
	}

	return S_OK;
}

void CNvClothCape::PriorityUpdate(_float)
{
}

void CNvClothCape::FixedUpdate(_float)
{
	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	auto* pPlayer = Cast<CPlayer>(pTarget);
	const _bool bOwnerRenderSuppressed =
		pPlayer && pPlayer->GetRenderInfluence();
	const _bool bSuppressionChanged =
		bOwnerRenderSuppressed !=
		m_bOwnerRenderSuppressed;

	// [LSY] 대시 중에는 망토가 보이지 않으므로 이동 관성을 누적하지 않는다.
	// 종료 시에는 현재 애니메이션 자세로 복원한 뒤 다시 표시한다.
	if (!UpdateAttachment(
			true,
			bOwnerRenderSuppressed ||
				bSuppressionChanged))
	{
		return;
	}

	if (bSuppressionChanged &&
		!ResetSimulationToAnimationPose())
	{
		return;
	}

	if (!UpdateBodyCollisions())
		return;

	m_bOwnerRenderSuppressed =
		bOwnerRenderSuppressed;
}

void CNvClothCape::Update(_float)
{
}

void CNvClothCape::LateUpdate(_float)
{
	if (!m_bRenderCape ||
		m_bOwnerRenderSuppressed)
		return;

	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hTarget);
	if (auto* pPlayer = Cast<CPlayer>(pTarget))
	{
		// [LSY] 플레이어가 대시 연출로 숨겨질 때 망토와 망토 그림자도 함께 숨긴다.
		if (pPlayer->GetRenderInfluence())
			return;
	}

	CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
	CGameInstance::Get().AddShadowRenderGroup(ACTORTYPE::DYNAMIC, this);
}

void CNvClothCape::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::PushID(this);

	ImGui::Checkbox(
		"Render Cape",
		&m_bRenderCape);

	auto vLocalPosition =
		GetTransform().GetPosition();
	if (ImGui::DragFloat3(
		"Cape Local Offset",
		&vLocalPosition.x,
		0.005f))
	{
		GetTransform().SetPosition(vLocalPosition);
		m_bSimulationTransformInitialized = false;
	}

	if (ImGui::TreeNode("Body Collision"))
	{
		ImGui::Checkbox(
			"Continuous Collision",
			&m_bContinuousBodyCollision);
		ImGui::DragFloat(
			"Collision Mass Scale",
			&m_fCollisionMassScale,
			0.1f,
			0.f,
			100.f);
		ImGui::DragFloat(
			"Collision Friction",
			&m_fCollisionFriction,
			0.01f,
			0.f,
			1.f);
		ImGui::Separator();
		ImGui::Checkbox(
			"Use Backstop",
			&m_bUseBackstop);
		ImGui::Checkbox(
			"Flip Backstop Normal",
			&m_bFlipBackstopNormal);
		ImGui::DragFloat(
			"Backstop Radius",
			&m_fBackstopRadius,
			0.005f,
			0.001f,
			20.f);
		ImGui::DragFloat(
			"Backstop Outward Offset",
			&m_fBackstopOffset,
			0.002f,
			-0.2f,
			0.2f);
		ImGui::DragFloat(
			"Backstop Full Ratio",
			&m_fBackstopFullRatio,
			0.01f,
			0.f,
			1.f);
		ImGui::DragFloat(
			"Backstop Fade End Ratio",
			&m_fBackstopFadeEndRatio,
			0.01f,
			0.f,
			1.f);
		ImGui::DragFloat(
			"Backstop Fade Depth",
			&m_fBackstopFadeDepth,
			0.01f,
			0.f,
			2.f);
		_bool bUseVirtualParticles =
			m_bUseVirtualParticles;
		if (ImGui::Checkbox(
				"Use Virtual Particles",
				&bUseVirtualParticles) &&
			m_pComNvCloth &&
			m_pComNvCloth->SetVirtualParticles(
				bUseVirtualParticles))
		{
			m_bUseVirtualParticles =
				bUseVirtualParticles;
		}
		ImGui::Checkbox(
			"Debug Body Collision",
			&m_bDebugBodyCollisions);
		ImGui::Text(
			"Collision Rig Shapes: %zu",
			m_BodyCollisionRig.Shapes.size());
		ImGui::Text(
			"NvCloth: %zu spheres, %zu capsules, "
			"%zu planes, %zu convexes",
			m_LastBodyCollisionDesc.vecSpheres.size(),
			m_LastBodyCollisionDesc.vecCapsules.size(),
			m_LastBodyCollisionDesc.vecPlanes.size(),
			m_LastBodyCollisionDesc.vecConvexes.size());

		if (m_BodyCollisionRig.Shapes.empty())
		{
			ImGui::Separator();
			ImGui::TextUnformatted(
				"Fallback Bone Spheres");
			for (auto& Binding :
				m_BodyCollisionBones)
			{
				ImGui::DragFloat(
					Binding.sBoneName.c_str(),
					&Binding.fRadius,
					0.005f,
					0.01f,
					1.f);
			}
		}
		ImGui::TreePop();
	}

	if (m_bDebugBodyCollisions)
		DebugDrawBodyCollisions();

	ImGui::PopID();
}

_bool CNvClothCape::ResolveAttachment()
{
	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return false;

	auto* pModelInstance =
		pTarget->GetComponent<CComModelInstance>(
			m_sTargetModelComponentTag);
	if (!pModelInstance ||
		!pModelInstance->GetModel())
	{
		return false;
	}

	m_iAttachBoneIndex =
		pModelInstance->GetModel()->
		Get_BoneIndex(m_sAttachBoneName.c_str());
	if (m_iAttachBoneIndex < 0)
		return false;

	const auto& SkinBoneNames =
		m_pClothMesh->GetSkinBoneNames();
	m_ResolvedSkinBoneIndices.resize(
		SkinBoneNames.size(),
		-1);
	m_SkinBoneToSimulationMatrices.resize(
		SkinBoneNames.size());
	size_t iResolvedSkinBoneCount{};
	for (size_t i = 0;
		i < SkinBoneNames.size();
		++i)
	{
		m_ResolvedSkinBoneIndices[i] =
			pModelInstance->GetModel()->
			Get_BoneIndex(
				SkinBoneNames[i].c_str());
		if (m_ResolvedSkinBoneIndices[i] >= 0)
			++iResolvedSkinBoneCount;
	}

	char szLog[256]{};
	sprintf_s(
		szLog,
		"[NvClothCape] Skin bones resolved: %zu / %zu. "
		"Particles without a matching bone use their Rest Pose.\n",
		iResolvedSkinBoneCount,
		SkinBoneNames.size());
	DEBUG_LOG(szLog);

	m_CollisionRigBoneIndices.resize(
		m_BodyCollisionRig.Shapes.size(),
		-1);
	size_t iResolvedCollisionShapeCount{};
	for (size_t i = 0;
		i < m_BodyCollisionRig.Shapes.size();
		++i)
	{
		m_CollisionRigBoneIndices[i] =
			pModelInstance->GetModel()->
			Get_BoneIndex(
				m_BodyCollisionRig.
					Shapes[i].sBoneName.c_str());
		if (m_CollisionRigBoneIndices[i] >= 0)
			++iResolvedCollisionShapeCount;
	}

	if (!m_BodyCollisionRig.Shapes.empty())
	{
		char szCollisionLog[256]{};
		sprintf_s(
			szCollisionLog,
			"[NvClothCape] Collision rig shapes resolved: "
			"%zu / %zu.\n",
			iResolvedCollisionShapeCount,
			m_BodyCollisionRig.Shapes.size());
		DEBUG_LOG(szCollisionLog);
	}

	for (auto& Binding : m_BodyCollisionBones)
	{
		Binding.iBoneIndex =
			pModelInstance->GetModel()->
			Get_BoneIndex(
				Binding.sBoneName.c_str());
		if (Binding.iBoneIndex < 0)
			return false;
	}

	return true;
}

_bool CNvClothCape::UpdateAnimationConstraints(
	CComModelInstance& ModelInstance,
	_fmatrix AttachmentWorld,
	_bool bResetPreviousParticles)
{
	if (!m_pComNvCloth ||
		!m_pClothMesh ||
		!ModelInstance.GetModel())
	{
		return false;
	}

	const auto& Bindings =
		m_pClothMesh->GetParticleSkinBindings();
	const auto& RestPositions =
		m_pClothMesh->GetFabricDesc().vecPositions;
	if (Bindings.empty() ||
		Bindings.size() != RestPositions.size() ||
		m_ResolvedSkinBoneIndices.size() !=
			m_pClothMesh->GetSkinBoneNames().size() ||
		m_SkinBoneToSimulationMatrices.size() !=
			m_ResolvedSkinBoneIndices.size())
	{
		return false;
	}

	_vector vDeterminant{};
	const _matrix InverseAttachmentWorld =
		XMMatrixInverse(
			&vDeterminant,
			AttachmentWorld);
	if (!std::isfinite(XMVectorGetX(vDeterminant)) ||
		std::fabs(XMVectorGetX(vDeterminant)) <=
			FLT_EPSILON)
	{
		return false;
	}

	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return false;

	const _matrix TargetWorld =
		pTarget->GetTransform().
		GetLoadedCombinedWorldMatrix();
	for (size_t i = 0;
		i < m_ResolvedSkinBoneIndices.size();
		++i)
	{
		const int32_t iTargetBoneIndex =
			m_ResolvedSkinBoneIndices[i];
		if (iTargetBoneIndex < 0)
		{
			XMStoreFloat4x4(
				&m_SkinBoneToSimulationMatrices[i],
				XMMatrixIdentity());
			continue;
		}

		_matrix BoneMatrix{};
		if (!GetTargetBoneMatrix(
			ModelInstance,
			iTargetBoneIndex,
			BoneMatrix))
		{
			return false;
		}

		_matrix BoneWorldRigid{};
		if (!NvClothCapeDetail::MakeRigidMatrix(
			BoneMatrix * TargetWorld,
			BoneWorldRigid))
		{
			return false;
		}

		XMStoreFloat4x4(
			&m_SkinBoneToSimulationMatrices[i],
			BoneWorldRigid *
				InverseAttachmentWorld);
	}

	auto& Desc = m_AnimationConstraintDesc;
	Desc.vecTargetPositions.resize(Bindings.size());
	Desc.vecMaxDistances.resize(Bindings.size());
	if (m_bUseBackstop)
	{
		Desc.vecSeparationCenters.resize(
			Bindings.size());
		Desc.vecSeparationRadii.resize(
			Bindings.size());
	}
	else
	{
		Desc.vecSeparationCenters.clear();
		Desc.vecSeparationRadii.clear();
	}
	float fMaxAnimationDistance{};
	for (const auto& Binding : Bindings)
	{
		fMaxAnimationDistance =
			std::max(
				fMaxAnimationDistance,
				Binding.fMaxDistance);
	}
	for (size_t i = 0;
		i < Bindings.size();
		++i)
	{
		_vector vTarget = XMVectorZero();
		_vector vNormal = XMVectorZero();
		float fTotalWeight{};
		for (const auto& Influence :
			Bindings[i].Influences)
		{
			if (Influence.fWeight <= 0.f ||
				Influence.iSourceBoneIndex >=
					m_SkinBoneToSimulationMatrices.size() ||
				m_ResolvedSkinBoneIndices[
					Influence.iSourceBoneIndex] < 0)
			{
				continue;
			}

			vTarget +=
				XMVector3TransformCoord(
					XMLoadFloat3(
						&Influence.vBoneLocalPosition),
					XMLoadFloat4x4(
						&m_SkinBoneToSimulationMatrices[
							Influence.iSourceBoneIndex])) *
				Influence.fWeight;
			vNormal +=
				XMVector3TransformNormal(
					XMLoadFloat3(
						&Influence.vBoneLocalNormal),
					XMLoadFloat4x4(
						&m_SkinBoneToSimulationMatrices[
							Influence.iSourceBoneIndex])) *
				Influence.fWeight;
			fTotalWeight += Influence.fWeight;
		}

		if (fTotalWeight > FLT_EPSILON)
		{
			vTarget /= fTotalWeight;
			vNormal /= fTotalWeight;
		}
		else
		{
			vTarget = XMLoadFloat3(&RestPositions[i]);
			vNormal = XMLoadFloat3(
				&Bindings[i].vRestSimulationNormal);
		}

		const float fNormalLengthSq =
			XMVectorGetX(
				XMVector3LengthSq(vNormal));
		if (!std::isfinite(fNormalLengthSq) ||
			fNormalLengthSq <= FLT_EPSILON)
		{
			vNormal = XMLoadFloat3(
				&Bindings[i].vRestSimulationNormal);
		}
		vNormal = XMVector3Normalize(vNormal);
		if (m_bFlipBackstopNormal)
			vNormal = -vNormal;

		XMStoreFloat3(
			&Desc.vecTargetPositions[i],
			vTarget);
		Desc.vecMaxDistances[i] =
			Bindings[i].fMaxDistance;
		if (m_bUseBackstop)
		{
			const float fRadius =
				std::max(m_fBackstopRadius, 0.001f);
			const float fDepthRatio =
				fMaxAnimationDistance > FLT_EPSILON ?
				std::clamp(
					Bindings[i].fMaxDistance /
						fMaxAnimationDistance,
					0.f,
					1.f) :
				0.f;
			const float fFullRatio =
				std::clamp(
					m_fBackstopFullRatio,
					0.f,
					1.f);
			const float fFadeEndRatio =
				std::clamp(
					std::max(
						m_fBackstopFadeEndRatio,
						fFullRatio + 0.001f),
					0.f,
					1.f);
			const float fLinearFade =
				std::clamp(
					(fDepthRatio - fFullRatio) /
						std::max(
							fFadeEndRatio -
								fFullRatio,
							0.001f),
					0.f,
					1.f);
			const float fSmoothFade =
				fLinearFade * fLinearFade *
				(3.f - 2.f * fLinearFade);
			const float fOutwardOffset =
				std::clamp(
					m_fBackstopOffset,
					-fRadius,
					fRadius * 0.95f);
			const float fFadeDepth =
				std::max(
					m_fBackstopFadeDepth,
					0.f) *
				fSmoothFade;
			const _vector vCenter =
				vTarget -
				vNormal *
					(fRadius -
						fOutwardOffset +
						fFadeDepth);
			XMStoreFloat3(
				&Desc.vecSeparationCenters[i],
				vCenter);
			Desc.vecSeparationRadii[i] =
				fRadius;
		}
	}

	Desc.bUpdateFixedParticles = true;
	Desc.bResetPreviousParticles =
		bResetPreviousParticles ||
		!m_bAnimationConstraintInitialized;
	if (!m_pComNvCloth->
		SetAnimationConstraints(Desc))
	{
		return false;
	}

	m_bAnimationConstraintInitialized = true;
	return true;
}

_bool CNvClothCape::GetTargetBoneMatrix(
	CComModelInstance& ModelInstance,
	int32_t iBoneIndex,
	_matrix& OutBoneMatrix) const
{
	if (iBoneIndex < 0 ||
		!ModelInstance.GetModel())
	{
		return false;
	}

	const auto iIndex =
		static_cast<size_t>(iBoneIndex);
	const auto& CombinedBones =
		ModelInstance.Get_CombinedBoneMatrices();
	if (iIndex < CombinedBones.size())
	{
		OutBoneMatrix =
			XMLoadFloat4x4(
				&CombinedBones[iIndex]);
		return true;
	}

	const auto& Bones =
		ModelInstance.GetModel()->GetBones();
	if (iIndex >= Bones.size() ||
		!Bones[iIndex])
	{
		return false;
	}

	OutBoneMatrix =
		Bones[iIndex]->
		Get_CombinedTransformationMatrix();
	return true;
}

_bool CNvClothCape::BuildBodyCollisionsFromRig(
	CComModelInstance& ModelInstance,
	_fmatrix TargetWorld,
	_fmatrix InverseSimulationWorld,
	NVCLOTH_COLLISION_DESC& OutDesc)
{
	if (m_BodyCollisionRig.Shapes.empty() ||
		m_CollisionRigBoneIndices.size() !=
			m_BodyCollisionRig.Shapes.size())
	{
		return false;
	}

	m_DebugBodyCollisionShapes.clear();
	for (size_t iShape = 0;
		iShape < m_BodyCollisionRig.Shapes.size();
		++iShape)
	{
		const int32_t iBoneIndex =
			m_CollisionRigBoneIndices[iShape];
		if (iBoneIndex < 0)
			continue;

		const auto& Shape =
			m_BodyCollisionRig.Shapes[iShape];
		if (!Shape.bEnabled)
			continue;

		_matrix BoneMatrix{};
		if (!GetTargetBoneMatrix(
			ModelInstance,
			iBoneIndex,
			BoneMatrix))
		{
			return false;
		}

		_matrix BoneWorld{};
		if (!NvClothCapeDetail::MakeRigidMatrix(
			BoneMatrix * TargetWorld,
			BoneWorld))
		{
			return false;
		}

		_matrix ShapeWorld{};
		if (!NvClothCapeDetail::MakeRigidMatrix(
			NvClothCapeDetail::MakePoseMatrix(
				Shape.vLocalPosition,
				Shape.vLocalRotation) *
			BoneWorld,
			ShapeWorld))
		{
			return false;
		}

		const _matrix ShapeToSimulation =
			ShapeWorld *
			InverseSimulationWorld;
		DEBUG_BODY_COLLISION_SHAPE DebugShape{};
		DebugShape.eType = Shape.eType;
		XMStoreFloat4x4(
			&DebugShape.SimulationPose,
			ShapeToSimulation);

		switch (Shape.eType)
		{
		case NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE:
		{
			if (OutDesc.vecSpheres.size() + 1 > 32)
				return false;

			NVCLOTH_COLLISION_SPHERE Sphere{};
			XMStoreFloat3(
				&Sphere.vCenter,
				XMVector3TransformCoord(
					XMVectorZero(),
					ShapeToSimulation));
			Sphere.fRadius =
				std::max(
					Shape.fRadius + Shape.fMargin,
					0.001f);
			OutDesc.vecSpheres.push_back(Sphere);
			DebugShape.fRadius = Sphere.fRadius;
			break;
		}

		case NVCLOTH_COLLISION_SHAPE_TYPE::CAPSULE:
		{
			if (OutDesc.vecSpheres.size() + 2 > 32)
				return false;

			const float fRadius =
				std::max(
					Shape.fRadius + Shape.fMargin,
					0.001f);
			const float fHalfHeight =
				std::max(Shape.fHalfHeight, 0.f);
			const uint32_t iSphere0 =
				static_cast<uint32_t>(
					OutDesc.vecSpheres.size());
			NVCLOTH_COLLISION_SPHERE Sphere0{};
			XMStoreFloat3(
				&Sphere0.vCenter,
				XMVector3TransformCoord(
					XMVectorSet(
						0.f,
						-fHalfHeight,
						0.f,
						1.f),
					ShapeToSimulation));
			Sphere0.fRadius = fRadius;
			OutDesc.vecSpheres.push_back(Sphere0);

			const uint32_t iSphere1 =
				static_cast<uint32_t>(
					OutDesc.vecSpheres.size());
			NVCLOTH_COLLISION_SPHERE Sphere1{};
			XMStoreFloat3(
				&Sphere1.vCenter,
				XMVector3TransformCoord(
					XMVectorSet(
						0.f,
						fHalfHeight,
						0.f,
						1.f),
					ShapeToSimulation));
			Sphere1.fRadius = fRadius;
			OutDesc.vecSpheres.push_back(Sphere1);
			OutDesc.vecCapsules.push_back({
				iSphere0,
				iSphere1
			});

			DebugShape.fRadius = fRadius;
			DebugShape.fHalfHeight = fHalfHeight;
			break;
		}

		case NVCLOTH_COLLISION_SHAPE_TYPE::BOX:
		{
			if (OutDesc.vecPlanes.size() + 6 > 32)
				return false;

			const _float3 vHalfExtents{
				std::max(
					Shape.vHalfExtents.x + Shape.fMargin,
					0.001f),
				std::max(
					Shape.vHalfExtents.y + Shape.fMargin,
					0.001f),
				std::max(
					Shape.vHalfExtents.z + Shape.fMargin,
					0.001f)
			};
			const _vector vCenter =
				XMVector3TransformCoord(
					XMVectorZero(),
					ShapeToSimulation);
			const _vector Axes[3]{
				XMVector3Normalize(
					XMVector3TransformNormal(
						XMVectorSet(
							1.f, 0.f, 0.f, 0.f),
						ShapeToSimulation)),
				XMVector3Normalize(
					XMVector3TransformNormal(
						XMVectorSet(
							0.f, 1.f, 0.f, 0.f),
						ShapeToSimulation)),
				XMVector3Normalize(
					XMVector3TransformNormal(
						XMVectorSet(
							0.f, 0.f, 1.f, 0.f),
						ShapeToSimulation))
			};
			const float Extents[3]{
				vHalfExtents.x,
				vHalfExtents.y,
				vHalfExtents.z
			};

			uint32_t iPlaneMask{};
			for (uint32_t iAxis = 0;
				iAxis < 3;
				++iAxis)
			{
				for (const float fSign :
					{ -1.f, 1.f })
				{
					const _vector vNormal =
						Axes[iAxis] * fSign;
					const _vector vPoint =
						vCenter +
						vNormal * Extents[iAxis];

					NVCLOTH_COLLISION_PLANE Plane{};
					XMStoreFloat3(
						&Plane.vNormal,
						vNormal);
					Plane.fDistance =
						-XMVectorGetX(
							XMVector3Dot(
								vNormal,
								vPoint));

					const uint32_t iPlane =
						static_cast<uint32_t>(
							OutDesc.vecPlanes.size());
					OutDesc.vecPlanes.push_back(
						Plane);
					iPlaneMask |= 1u << iPlane;
				}
			}
			OutDesc.vecConvexes.push_back({
				iPlaneMask
			});
			DebugShape.vHalfExtents = vHalfExtents;
			break;
		}
		}

		m_DebugBodyCollisionShapes.push_back(
			DebugShape);
	}

	return !m_DebugBodyCollisionShapes.empty();
}

void CNvClothCape::DebugDrawBodyCollisions()
{
	auto* pDebug =
		CGameInstance::Get().GetDbgLineRender();
	if (!pDebug)
		return;

	const auto PreviousDepthMode =
		pDebug->GetDepthMode();
	const auto PreviousColor =
		pDebug->GetColor();
	pDebug->SetDepthTest(false);
	pDebug->SetColor(
		{ 0.1f, 0.9f, 1.f, 1.f });

	const _matrix SimulationWorld =
		GetTransform().
		GetLoadedCombinedWorldMatrix();
	for (const auto& Shape :
		m_DebugBodyCollisionShapes)
	{
		const _matrix ShapeWorld =
			XMLoadFloat4x4(
				&Shape.SimulationPose) *
			SimulationWorld;
		switch (Shape.eType)
		{
		case NVCLOTH_COLLISION_SHAPE_TYPE::BOX:
			pDebug->AddBox(
				Shape.vHalfExtents,
				ShapeWorld);
			break;
		case NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE:
			pDebug->AddSphere(
				Shape.fRadius,
				ShapeWorld);
			break;
		case NVCLOTH_COLLISION_SHAPE_TYPE::CAPSULE:
			pDebug->AddCapsule(
				Shape.fRadius,
				Shape.fHalfHeight,
				ShapeWorld);
			break;
		}
	}

	pDebug->SetColor(PreviousColor);
	pDebug->SetDepthMode(PreviousDepthMode);
}

_bool CNvClothCape::UpdateBodyCollisions()
{
	if (!m_pComNvCloth)
		return false;

	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return false;

	auto* pModelInstance =
		pTarget->GetComponent<CComModelInstance>(
			m_sTargetModelComponentTag);
	if (!pModelInstance ||
		!pModelInstance->GetModel())
	{
		return false;
	}

	_matrix SimulationWorld{};
	if (!NvClothCapeDetail::MakeRigidMatrix(
		GetTransform().
			GetLoadedCombinedWorldMatrix(),
		SimulationWorld))
	{
		return false;
	}

	_vector vDeterminant{};
	const _matrix InverseSimulationWorld =
		XMMatrixInverse(
			&vDeterminant,
			SimulationWorld);
	const float fDeterminant =
		XMVectorGetX(vDeterminant);
	if (!std::isfinite(fDeterminant) ||
		std::abs(fDeterminant) <= 1.e-8f)
	{
		return false;
	}

	const _matrix TargetWorld =
		pTarget->GetTransform().
		GetLoadedCombinedWorldMatrix();

	NVCLOTH_COLLISION_DESC CollisionDesc{};
	CollisionDesc.bContinuousCollision =
		m_bContinuousBodyCollision;
	CollisionDesc.fCollisionMassScale =
		m_fCollisionMassScale;
	CollisionDesc.fFriction =
		m_fCollisionFriction;

	if (!m_BodyCollisionRig.Shapes.empty())
	{
		if (!BuildBodyCollisionsFromRig(
			*pModelInstance,
			TargetWorld,
			InverseSimulationWorld,
			CollisionDesc))
		{
			return false;
		}
	}
	else
	{
		m_DebugBodyCollisionShapes.clear();
		CollisionDesc.vecSpheres.reserve(
			m_BodyCollisionBones.size());
		for (const auto& Binding :
			m_BodyCollisionBones)
		{
			_matrix BoneMatrix{};
			if (!GetTargetBoneMatrix(
				*pModelInstance,
				Binding.iBoneIndex,
				BoneMatrix))
			{
				return false;
			}

			const _vector vWorldPosition =
				XMVector3TransformCoord(
					XMVectorZero(),
					BoneMatrix * TargetWorld);
			const _vector vLocalPosition =
				XMVector3TransformCoord(
					vWorldPosition,
					InverseSimulationWorld);

			NVCLOTH_COLLISION_SPHERE Sphere{};
			XMStoreFloat3(
				&Sphere.vCenter,
				vLocalPosition);
			Sphere.fRadius = Binding.fRadius;
			CollisionDesc.vecSpheres.push_back(
				Sphere);

			DEBUG_BODY_COLLISION_SHAPE DebugShape{};
			DebugShape.eType =
				NVCLOTH_COLLISION_SHAPE_TYPE::SPHERE;
			DebugShape.fRadius =
				Sphere.fRadius;
			XMStoreFloat4x4(
				&DebugShape.SimulationPose,
				XMMatrixTranslationFromVector(
					vLocalPosition));
			m_DebugBodyCollisionShapes.push_back(
				DebugShape);
		}

		CollisionDesc.vecCapsules = {
			{ 0, 1 },
			{ 1, 2 },
			{ 1, 3 },
			{ 1, 4 }
		};
	}

	if (!m_pComNvCloth->SetCollisions(
		CollisionDesc))
	{
		return false;
	}

	m_LastBodyCollisionDesc =
		std::move(CollisionDesc);
	return true;
}

_bool CNvClothCape::ResetSimulationToAnimationPose()
{
	if (!m_pComNvCloth ||
		m_AnimationConstraintDesc.
			vecTargetPositions.empty())
	{
		return false;
	}

	return m_pComNvCloth->ResetParticlesToPositions(
		m_AnimationConstraintDesc.vecTargetPositions);
}

_bool CNvClothCape::UpdateAttachment(
	_bool bUpdateSimulation,
	_bool bForceTeleport)
{
	auto* pTarget =
		CGameInstance::Get().
		GetGameObjectByHandle(m_hTarget);
	if (!pTarget)
		return false;

	auto* pModelInstance =
		pTarget->GetComponent<CComModelInstance>(
			m_sTargetModelComponentTag);
	if (!pModelInstance ||
		!pModelInstance->GetModel())
	{
		return false;
	}

	if (m_iAttachBoneIndex < 0 &&
		!ResolveAttachment())
	{
		return false;
	}

	const auto iBoneIndex =
		static_cast<size_t>(m_iAttachBoneIndex);
	const auto& CombinedBones =
		pModelInstance->Get_CombinedBoneMatrices();
	_matrix BoneMatrix{};
	if (iBoneIndex < CombinedBones.size())
	{
		BoneMatrix =
			XMLoadFloat4x4(
				&CombinedBones[iBoneIndex]);
	}
	else
	{
		const auto& Bones =
			pModelInstance->GetModel()->GetBones();
		if (iBoneIndex >= Bones.size() ||
			!Bones[iBoneIndex])
		{
			return false;
		}
		BoneMatrix =
			Bones[iBoneIndex]->
			Get_CombinedTransformationMatrix();
	}

	_matrix AttachmentWorld{};
	if (!NvClothCapeDetail::MakeRigidMatrix(
		BoneMatrix *
			pTarget->GetTransform().
			GetLoadedCombinedWorldMatrix(),
		AttachmentWorld))
	{
		return false;
	}

	XMStoreFloat4x4(
		&m_ParentWorld,
		AttachmentWorld);
	GetTransform().SetParentWorldMatrix(
		m_ParentWorld);
	GetTransform().Update();
	m_bAttachmentInitialized = true;

	if (!bUpdateSimulation ||
		!m_pComNvCloth)
	{
		return true;
	}

	_matrix SimulationWorld{};
	if (!NvClothCapeDetail::MakeRigidMatrix(
		GetTransform().
			GetLoadedCombinedWorldMatrix(),
		SimulationWorld))
	{
		return false;
	}

	_vector vScale{};
	_vector qRotation{};
	_vector vTranslation{};
	if (!XMMatrixDecompose(
		&vScale,
		&qRotation,
		&vTranslation,
		SimulationWorld))
	{
		return false;
	}

	_float3 vCurrentPosition{};
	_float4 vCurrentRotation{};
	XMStoreFloat3(
		&vCurrentPosition,
		vTranslation);
	XMStoreFloat4(
		&vCurrentRotation,
		XMQuaternionNormalize(qRotation));

	const float fDistance =
		XMVectorGetX(XMVector3Length(
			XMLoadFloat3(&vCurrentPosition) -
			XMLoadFloat3(
				&m_vPreviousAttachPosition)));
	const _bool bTeleport =
		bForceTeleport ||
		!m_bSimulationTransformInitialized ||
		fDistance > m_fTeleportDistance;
	if (!m_pComNvCloth->SetSimulationTransform(
		vCurrentPosition,
		vCurrentRotation,
		bTeleport))
	{
		return false;
	}

	if (!UpdateAnimationConstraints(
		*pModelInstance,
		AttachmentWorld,
		bTeleport))
	{
		return false;
	}

	m_vPreviousAttachPosition =
		vCurrentPosition;
	m_bSimulationTransformInitialized = true;
	return true;
}

HRESULT CNvClothCape::Render(
	ID3D11DeviceContext* pContext,
	const RENDER_CTX& Context)
{
	if (!pContext ||
		!m_pComCBufferPerObject ||
		!m_pComModelInstance ||
		!m_pComNvCloth ||
		!m_pClothMesh ||
		!m_pVertexShader ||
		!m_pPixelShader)
	{
		return E_FAIL;
	}

	NVCLOTH_RENDER_PARTICLE_VIEW ParticleView{};
	if (!m_pComNvCloth->GetRenderParticleView(
		ParticleView) ||
		ParticleView.iParticleCount !=
			m_pClothMesh->GetParticleCount())
	{
		return E_FAIL;
	}

	CB_PER_OBJECT PerObject{};
	PerObject.matWorld =
		*GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(
		&PerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() *
			Context.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(
		pContext,
		&PerObject,
		sizeof(PerObject))))
	{
		return E_FAIL;
	}

	pContext->VSSetConstantBuffers(
		ETOUI(B_SLOTNUMBER::PER_OBJECT),
		1,
		m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(
		ETOUI(B_SLOTNUMBER::PER_OBJECT),
		1,
		m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->IASetInputLayout(
		m_pVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(
		m_pVertexShader->GetVertexShader().Get(),
		nullptr,
		0);
	pContext->PSSetShader(
		m_pPixelShader->GetPixelShader().Get(),
		nullptr,
		0);
	pContext->VSSetShaderResources(
		9,
		1,
		&ParticleView.pSRV);

	const auto& Sections =
		m_pClothMesh->GetSections();
	for (uint32_t i = 0;
		i < Sections.size();
		++i)
	{
		const auto& Section = Sections[i];
		if (!Section.pVIBuffer)
			continue;

		ID3D11Buffer* pVertexBuffer =
			Section.pVIBuffer->
				GetVertexBuffer().Get();
		const uint32_t iVertexStride =
			Section.pVIBuffer->
				GetVertexStride();
		const uint32_t iOffset{};
		pContext->IASetVertexBuffers(
			0,
			1,
			&pVertexBuffer,
			&iVertexStride,
			&iOffset);
		pContext->IASetIndexBuffer(
			Section.pVIBuffer->
				GetIndexBuffer().Get(),
			Section.pVIBuffer->
				GetIndexFormat(),
			0);
		pContext->IASetPrimitiveTopology(
			Section.pVIBuffer->
				GetPrimitiveType());

		m_pComModelInstance->Bind_Textures(
			pContext,
			Section.iSourceMeshIndex);
		m_pComModelInstance->Bind_Materials(
			pContext,
			{ 1.f, 1.f, 1.f },
			0.f,
			{ 1.f, 1.f, 1.f },
			0.f,
			1.f);
		pContext->DrawIndexed(
			Section.pVIBuffer->
				GetNumIndices(),
			0,
			0);
	}

	ID3D11ShaderResourceView* pNullSRV{};
	pContext->VSSetShaderResources(
		9,
		1,
		&pNullSRV);
	return S_OK;
}

HRESULT CNvClothCape::Render_Shadow(
	ID3D11DeviceContext* pContext,
	const RENDER_CTX& Context)
{
	if (!pContext ||
		!m_pComCBufferPerObject ||
		!m_pComNvCloth ||
		!m_pClothMesh ||
		!m_pShadowVertexShader ||
		!m_pPointShadowVertexShader)
	{
		return E_FAIL;
	}

	NVCLOTH_RENDER_PARTICLE_VIEW ParticleView{};
	if (!m_pComNvCloth->GetRenderParticleView(
		ParticleView) ||
		ParticleView.iParticleCount !=
			m_pClothMesh->GetParticleCount())
	{
		return E_FAIL;
	}

	CB_PER_OBJECT PerObject{};
	PerObject.matWorld =
		*GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(
		&PerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() *
			Context.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(
		pContext,
		&PerObject,
		sizeof(PerObject))))
	{
		return E_FAIL;
	}

	ComPtr<ID3D11InputLayout> pPreviousInputLayout{};
	ComPtr<ID3D11VertexShader> pPreviousVertexShader{};
	ComPtr<ID3D11PixelShader> pPreviousPixelShader{};
	ComPtr<ID3D11ShaderResourceView>
		pPreviousParticleSRV{};
	pContext->IAGetInputLayout(
		pPreviousInputLayout.GetAddressOf());
	pContext->VSGetShader(
		pPreviousVertexShader.GetAddressOf(),
		nullptr,
		nullptr);
	/*----------- 광윤 추가 -----------*/
	pContext->PSGetShader(pPreviousPixelShader.GetAddressOf(),nullptr,nullptr);
	/*---------------------------------*/
	/*----------- 광윤 수정 -----------*/
	//ComPtr<ID3D11GeometryShader> pPreviousGeometryShader{};
	//pContext->GSGetShader(
	//	pPreviousGeometryShader.GetAddressOf(),
	//	nullptr,
	//	nullptr);
	/*---------------------------------*/
	pContext->VSGetShaderResources(
		9,
		1,
		pPreviousParticleSRV.GetAddressOf());

	pContext->VSSetConstantBuffers(
		ETOUI(B_SLOTNUMBER::PER_OBJECT),
		1,
		m_pComCBufferPerObject->GetAdressOfBuffer());
	// Point Shadow는 LightManager의 GS가 월드 좌표를 큐브맵 6면으로 확장한다.
	// GS가 없는 Directional/Spot Shadow는 기존 SV_POSITION 출력 VS를 사용한다.
	//const auto& pShadowVertexShader =
	//	pPreviousGeometryShader ?
	//	m_pPointShadowVertexShader :
	//	m_pShadowVertexShader;
	/*----------- 광윤 추가 -----------*/	// 기존 로직 변경되어서 아래 코드로 변경
	const bool IsPointFaceShadow =
		Context.PointShadowFaceIndex >= 0;

	const auto& pShadowVertexShader =
		IsPointFaceShadow
		? m_pPointShadowVertexShader
		: m_pShadowVertexShader;
	/*---------------------------------*/
	pContext->IASetInputLayout(
		pShadowVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(
		pShadowVertexShader->GetVertexShader().Get(),
		nullptr,
		0);
	/*----------- 광윤 추가 -----------*/
	pContext->PSSetShader(nullptr, nullptr, 0);
	/*---------------------------------*/
	pContext->VSSetShaderResources(
		9,
		1,
		&ParticleView.pSRV);

	for (const auto& Section :
		m_pClothMesh->GetSections())
	{
		if (!Section.pVIBuffer)
			continue;

		ID3D11Buffer* pVertexBuffer =
			Section.pVIBuffer->
				GetVertexBuffer().Get();
		const uint32_t iVertexStride =
			Section.pVIBuffer->GetVertexStride();
		const uint32_t iOffset{};
		pContext->IASetVertexBuffers(
			0,
			1,
			&pVertexBuffer,
			&iVertexStride,
			&iOffset);
		pContext->IASetIndexBuffer(
			Section.pVIBuffer->GetIndexBuffer().Get(),
			Section.pVIBuffer->GetIndexFormat(),
			0);
		pContext->IASetPrimitiveTopology(
			Section.pVIBuffer->GetPrimitiveType());
		pContext->DrawIndexed(
			Section.pVIBuffer->GetNumIndices(),
			0,
			0);
	}

	pContext->VSSetShaderResources(
		9,
		1,
		pPreviousParticleSRV.GetAddressOf());
	pContext->IASetInputLayout(
		pPreviousInputLayout.Get());
	pContext->VSSetShader(
		pPreviousVertexShader.Get(),
		nullptr,
		0);
	/*----------- 광윤 추가 -----------*/
	pContext->PSSetShader(pPreviousPixelShader.Get(), nullptr, 0);
	/*---------------------------------*/
	return S_OK;
}

bool CNvClothCape::GetShadowBounds(BoundingBox& OutBounds) const {
	CGameObject* pTarget = CGameInstance::Get().GetGameObjectByHandle(m_hTarget);

	if (pTarget == nullptr)	return false;

	if (pTarget->GetShadowBounds(OutBounds))	{
		OutBounds.Extents.x += 0.5f;
		OutBounds.Extents.y += 0.25f;
		OutBounds.Extents.z += 1.0f;

		return true;
	}

	OutBounds.Center =pTarget->GetTransform().GetPosition();
	OutBounds.Extents = { 1.75f, 2.0f, 2.0f };

	return true;
}

UPtr<CNvClothCape> CNvClothCape::Create()
{
	auto pInstance =
		ToUPtr(new CNvClothCape{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CNvClothCape");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CNvClothCape::Clone(void* pArg)
{
	auto pInstance =
		ToUPtr(new CNvClothCape{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CNvClothCape");
		return nullptr;
	}
	return pInstance;
}
