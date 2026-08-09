#include "pch.h"
#include "GameInstance.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
#include "ResModelAnim.h"
#include "ResModel.h"
NS_USING(Engine)



void CComAnimator::UpdateGUI()
{
	CComponent::UpdateGUI();

	ImGui::Separator();
	ImGui::TextUnformatted("Extracted Root Motion");
	ImGui::Text("Enabled: %s", m_bRootMotion ? "true" : "false");
	ImGui::Text("Root Bone Index: %d", m_iRootBoneIndex);
	ImGui::Text(
		"Translation Delta: %.5f, %.5f, %.5f",
		m_vRootMotionDelta.x,
		m_vRootMotionDelta.y,
		m_vRootMotionDelta.z);
	ImGui::Text(
		"Rotation Delta (Quaternion): %.5f, %.5f, %.5f, %.5f",
		m_qRootMotionRotationDelta.x,
		m_qRootMotionRotationDelta.y,
		m_qRootMotionRotationDelta.z,
		m_qRootMotionRotationDelta.w);

	const _vector qRotation = XMQuaternionNormalize(
		XMLoadFloat4(&m_qRootMotionRotationDelta));
	_vector vRotationAxis{};
	_float fRotationAngle{};
	XMQuaternionToAxisAngle(
		&vRotationAxis,
		&fRotationAngle,
		qRotation);

	_float3 vRotationAxisValue{};
	XMStoreFloat3(&vRotationAxisValue, vRotationAxis);
	ImGui::Text(
		"Rotation Delta (Axis): %.5f, %.5f, %.5f",
		vRotationAxisValue.x,
		vRotationAxisValue.y,
		vRotationAxisValue.z);
	ImGui::Text(
		"Rotation Delta (Angle): %.3f deg",
		XMConvertToDegrees(fRotationAngle));

	_float4x4 rootMotionTransform{};
	XMStoreFloat4x4(
		&rootMotionTransform,
		XMMatrixRotationQuaternion(qRotation) *
		XMMatrixTranslation(
			m_vRootMotionDelta.x,
			m_vRootMotionDelta.y,
			m_vRootMotionDelta.z));

	ImGui::TextUnformatted("Delta Transform");
	ImGui::Text(
		"[ %.5f  %.5f  %.5f  %.5f ]",
		rootMotionTransform._11,
		rootMotionTransform._12,
		rootMotionTransform._13,
		rootMotionTransform._14);
	ImGui::Text(
		"[ %.5f  %.5f  %.5f  %.5f ]",
		rootMotionTransform._21,
		rootMotionTransform._22,
		rootMotionTransform._23,
		rootMotionTransform._24);
	ImGui::Text(
		"[ %.5f  %.5f  %.5f  %.5f ]",
		rootMotionTransform._31,
		rootMotionTransform._32,
		rootMotionTransform._33,
		rootMotionTransform._34);
	ImGui::Text(
		"[ %.5f  %.5f  %.5f  %.5f ]",
		rootMotionTransform._41,
		rootMotionTransform._42,
		rootMotionTransform._43,
		rootMotionTransform._44);

	//if (ImGui::Button("save")) {
	//	CGameInstance::Get( ).JsonSerialize("./Test.json", m_CurAnimState);
	//}
	//if (ImGui::Button("load")) {
	//	ANIMSTRUCT m_Cur;
	//	CGameInstance::Get().JsonDeSerialize("./Test.json", m_Cur);
	//	Play_Anim(m_Cur.iAnimIndex, m_Cur.bLoop);
	//	m_bPlay = true;
	//}
}

CComAnimator::CComAnimator()
{


}

CComAnimator::~CComAnimator()
{
	char szLog[128]{};
	sprintf_s(szLog, "[Destroy] CBTThinkAnimMonster: %p\n", static_cast<void*>(this));
	OutputDebugStringA(szLog);

}


HRESULT CComAnimator::Initialize(void* pArg)
{
    if (FAILED(CComponent::Initialize(pArg)))
    {
        return E_FAIL;
    }

    if (pArg != nullptr) {
        CComAnimator::DESC* pDesc = reinterpret_cast<CComAnimator::DESC*>(pArg);
           
        m_Comtag = pDesc->sComTag;
        m_pModelInstance = GetGameObject()->GetComponent<CComModelInstance>(m_Comtag);
		if (nullptr == m_pModelInstance) return E_FAIL;

		int32_t iIndex{-1};
		iIndex = m_pModelInstance->GetModel()->Get_BoneIndex("Reference");
		if (iIndex != -1)
		{
			m_iRootBoneIndex = iIndex;
			return S_OK;
		}
		iIndex = m_pModelInstance->GetModel()->Get_BoneIndex("root");
		if (iIndex != -1)
		{
			m_iRootBoneIndex = iIndex;
			return S_OK;
		}
	
    }



    return S_OK;
}

HRESULT CComAnimator::Update(_float fTimeDelta)
{

    if (m_bPlay) {
        switch (m_iPlayAnimationType) {
        case ANIMTYPE::ANIM:
            if (m_eEvaluationMode == EVALUATION_MODE::GPU) Update_Anim_GPU(fTimeDelta);
            else if (m_eEvaluationMode == EVALUATION_MODE::CPU_GPU) Update_Anim_CPU_GPU(fTimeDelta);
            else Update_Anim(fTimeDelta);
            break;
        case ANIMTYPE::ACTION:
            if (m_eEvaluationMode == EVALUATION_MODE::GPU) Update_Action_GPU(fTimeDelta);
            else if (m_eEvaluationMode == EVALUATION_MODE::CPU_GPU) Update_Action_CPU_GPU(fTimeDelta);
            else Update_Action(fTimeDelta);
            break;
        }
    }


    return S_OK;
}

HRESULT CComAnimator::Update_Anim(_float fTimeDelta)
{
	if (m_pModelInstance == nullptr)
		return E_FAIL;

	auto pModel = m_pModelInstance->GetModel();
	if (pModel == nullptr)
		return E_FAIL;

	auto& Anims = pModel->GetAnimations();

	if (m_CurAnimState.iAnimIndex < 0 ||
		m_CurAnimState.iAnimIndex >= static_cast<int32_t>(Anims.size()))
	{
		return E_FAIL;
	}

	auto pAnim = Anims[m_CurAnimState.iAnimIndex];
	if (pAnim == nullptr)
		return E_FAIL;


	_float fPrevTrackPosition = m_CurAnimState.fTrackPosition;

	Update_AnimState(fTimeDelta, m_CurAnimState);

	m_vRootMotionDelta = _float3{ 0.f, 0.f, 0.f };
	m_qRootMotionRotationDelta = _float4{ 0.f, 0.f, 0.f, 1.f };
	if (m_CurAnimState.fTrackPosition < fPrevTrackPosition)
	{
		Invalidate_RootMotionCache();
		Prepare_RootMotionCache(pAnim.get(), m_CurAnimState.fTrackPosition);
	}
	else
	{
		Prepare_RootMotionCache(pAnim.get(), fPrevTrackPosition);
	}
	Build_BoneMatrices_CPU(fTimeDelta);

	if (pAnim->GetDuration() > 0.f)
	{
		m_fRatio = m_CurAnimState.fTrackPosition / pAnim->GetDuration();
	}
	else
	{
		m_fRatio = 0.f;
	}

	return S_OK;


}

HRESULT CComAnimator::Update_Action(_float fTimeDelta)
{
	if (m_pModelInstance == nullptr)
		return E_FAIL;

	auto pModel = m_pModelInstance->GetModel();
	if (pModel == nullptr)
		return E_FAIL;

	auto& Anims = pModel->GetAnimations();

	if (m_CurAnimState.iAnimIndex < 0 ||m_CurAnimState.iAnimIndex >= static_cast<int32_t>(Anims.size()))
	{
		return E_FAIL;
	}

	auto pResAnim = Anims[m_CurAnimState.iAnimIndex];
	if (pResAnim == nullptr)
		return E_FAIL;


	_float fPrevTrackPosition = m_CurAnimState.fTrackPosition;
	const int32_t iPreviousAnimIndex = m_CurAnimState.iAnimIndex;

	Update_ActionState(fTimeDelta, m_CurAnimState);
	if (m_CurAnimState.iAnimIndex != iPreviousAnimIndex)
	{
		Invalidate_RootMotionCache();
		if (m_CurAnimState.iAnimIndex < 0 ||
			m_CurAnimState.iAnimIndex >= static_cast<int32_t>(Anims.size()))
		{
			return E_FAIL;
		}
		pResAnim = Anims[m_CurAnimState.iAnimIndex];
		if (pResAnim == nullptr)
			return E_FAIL;
	}

	m_vRootMotionDelta = _float3{ 0.f, 0.f, 0.f };
	m_qRootMotionRotationDelta = _float4{ 0.f, 0.f, 0.f, 1.f };
	if (m_CurAnimState.iAnimIndex != iPreviousAnimIndex ||
		m_CurAnimState.fTrackPosition < fPrevTrackPosition)
	{
		Invalidate_RootMotionCache();
		Prepare_RootMotionCache(pResAnim.get(), m_CurAnimState.fTrackPosition);
	}
	else
	{
		Prepare_RootMotionCache(pResAnim.get(), fPrevTrackPosition);
	}
	Build_BoneMatrices_CPU(fTimeDelta);

	if (pResAnim->GetDuration() > 0.f)
	{
		m_fRatio = m_CurAnimState.fTrackPosition / pResAnim->GetDuration();
	}
	else
	{
		m_fRatio = 0.f;
	}

	return S_OK;

}

HRESULT CComAnimator::Update_Anim_CPU_GPU(_float fTimeDelta)
{
	return Update_Anim(fTimeDelta);
}

HRESULT CComAnimator::Update_Action_CPU_GPU(_float fTimeDelta)
{
	return Update_Action(fTimeDelta);
}

HRESULT CComAnimator::Update_Anim_GPU(_float fTimeDelta) {
	if (m_pModelInstance == nullptr)
		return E_FAIL;

	auto pModel = m_pModelInstance->GetModel();
	if (pModel == nullptr)
		return E_FAIL;

	auto& Anims = pModel->GetAnimations();

	if (m_CurAnimState.iAnimIndex < 0 ||
		m_CurAnimState.iAnimIndex >= static_cast<int32_t>(Anims.size()))
	{
		return E_FAIL;
	}

	auto pAnim = Anims[m_CurAnimState.iAnimIndex];
	if (pAnim == nullptr)
		return E_FAIL;


	_float fPrevTrackPosition = m_CurAnimState.fTrackPosition;

	Update_AnimState(fTimeDelta, m_CurAnimState);
	Advance_GPUBlend(fTimeDelta);

	Update_RootMotion_GPU(pAnim.get(), fPrevTrackPosition);


	if (pAnim->GetDuration() > 0.f)
	{
		m_fRatio = m_CurAnimState.fTrackPosition / pAnim->GetDuration();
	}
	else
	{
		m_fRatio = 0.f;
	}

	return S_OK;


}

HRESULT	CComAnimator::Update_Action_GPU(_float fTimeDelta) {

	if (m_pModelInstance == nullptr)
		return E_FAIL;

	auto pModel = m_pModelInstance->GetModel();
	if (pModel == nullptr)
		return E_FAIL;

	auto& Anims = pModel->GetAnimations();

	if (m_CurAnimState.iAnimIndex < 0 || m_CurAnimState.iAnimIndex >= static_cast<int32_t>(Anims.size()))
	{
		return E_FAIL;
	}

	auto pResAnim = Anims[m_CurAnimState.iAnimIndex];
	if (pResAnim == nullptr)
		return E_FAIL;


	_float fPrevTrackPosition = m_CurAnimState.fTrackPosition;
	const int32_t iPreviousAnimIndex = m_CurAnimState.iAnimIndex;

	Update_ActionState(fTimeDelta, m_CurAnimState);
	Advance_GPUBlend(fTimeDelta);
	if (m_CurAnimState.iAnimIndex != iPreviousAnimIndex)
	{
		Invalidate_RootMotionCache();
		if (m_CurAnimState.iAnimIndex < 0 ||
			m_CurAnimState.iAnimIndex >= static_cast<int32_t>(Anims.size()))
		{
			return E_FAIL;
		}
		pResAnim = Anims[m_CurAnimState.iAnimIndex];
		if (pResAnim == nullptr)
			return E_FAIL;
		fPrevTrackPosition = m_CurAnimState.fTrackPosition;
	}

	Update_RootMotion_GPU(pResAnim.get(), fPrevTrackPosition);
	if (pResAnim->GetDuration() > 0.f)
	{
		m_fRatio = m_CurAnimState.fTrackPosition / pResAnim->GetDuration();
	}
	else
	{
		m_fRatio = 0.f;
	}

	return S_OK;
}

void CComAnimator::Play_Anim(int32_t iAnimIndex, _bool bLoop, _float fBlendDuration)
{
	if (m_pModelInstance == nullptr)
		return;

	auto pModel = m_pModelInstance->GetModel();
	if (pModel == nullptr)
		return;

	auto& Anims = pModel->GetAnimations();

	if (iAnimIndex < 0 || iAnimIndex >= static_cast<int32_t>(Anims.size()))
		return;

	// 같은 애니메이션이면 다시 세팅하지 않음
	if (m_CurAnimState.iAnimIndex == iAnimIndex)
		return;

	// 이전 상태 저장
	m_PrevAnimState = m_CurAnimState;

	// 현재 상태 새로 세팅
	m_CurAnimState.Reset();

	m_CurAnimState.iAnimIndex = iAnimIndex;
	m_CurAnimState.fTrackPosition = 0.f;
	m_CurAnimState.fSpeed = 1.f;
	m_CurAnimState.bLoop = bLoop;
	m_CurAnimState.bFinished = false;

	// 해당 애니메이션 채널 수만큼 키프레임 인덱스 준비
	m_CurAnimState.KeyFrameIndices.resize(Anims[iAnimIndex]->GetNumChannel(),0);

	// 블렌딩 시작 여부
	if (m_PrevAnimState.iAnimIndex >= 0 && fBlendDuration > 0.f)
	{
		m_bBlending = true;
		m_fBlendTime = 0.f;
		m_fBlendDuration = fBlendDuration;

		// 복사 
		m_BlendStartLocalMatrices = m_LocalBoneMatrices;
	}
	else
	{
		m_bBlending = false;
		m_fBlendTime = 0.f;
		m_fBlendDuration = 0.f;
	
	}
	m_fRatio = 0.f;
	m_iPlayAnimationType = ANIMTYPE::ANIM;
	m_bPlay = true;
	Invalidate_RootMotionCache();
}

void CComAnimator::Play_UpperAnim(int32_t iAnimIndex, _bool bLoop, _float fFadeDuration)
{
	if (m_pModelInstance == nullptr || m_pModelInstance->GetModel() == nullptr)
		return;

	auto& Anims = m_pModelInstance->GetModel()->GetAnimations();
	if (iAnimIndex < 0 || iAnimIndex >= static_cast<int32_t>(Anims.size()) || Anims[iAnimIndex] == nullptr)
		return;

	m_UpperAnimState.Reset();
	m_UpperAnimState.SetAnim(iAnimIndex, bLoop);
	m_UpperAnimState.KeyFrameIndices.resize(Anims[iAnimIndex]->GetNumChannel(), 0);

	m_fUpperFadeStartWeight = m_fUpperLayerWeight;
	m_fUpperFadeTargetWeight = 1.f;
	m_fUpperFadeTime = 0.f;
	m_fUpperFadeDuration = std::max(fFadeDuration, 0.f);
	if (m_fUpperFadeDuration <= 0.f)
		m_fUpperLayerWeight = 1.f;
}

void CComAnimator::Stop_UpperAnim(_float fFadeDuration)
{
	if (!m_UpperAnimState.IsValid())
		return;

	m_fUpperFadeStartWeight = m_fUpperLayerWeight;
	m_fUpperFadeTargetWeight = 0.f;
	m_fUpperFadeTime = 0.f;
	m_fUpperFadeDuration = std::max(fFadeDuration, 0.f);
	if (m_fUpperFadeDuration <= 0.f)
	{
		m_fUpperLayerWeight = 0.f;
		m_UpperAnimState.Reset();
	}
}

_bool CComAnimator::Set_UpperBodyRootBone(const _char* pBoneName, uint32_t iBlendDepth)
{
	if (m_pModelInstance == nullptr || m_pModelInstance->GetModel() == nullptr || pBoneName == nullptr)
		return false;

	auto pModel = m_pModelInstance->GetModel();
	const int32_t iRootBoneIndex = pModel->Get_BoneIndex(pBoneName);
	const auto& Bones = pModel->GetBones();
	if (iRootBoneIndex < 0 || iRootBoneIndex >= static_cast<int32_t>(Bones.size()))
		return false;

	m_UpperBodyMask.assign(Bones.size(), 0.f);
	const uint32_t iSafeBlendDepth = std::max(iBlendDepth, 1u);

	for (uint32_t i = 0; i < static_cast<uint32_t>(Bones.size()); ++i)
	{
		int32_t iCurrent = static_cast<int32_t>(i);
		uint32_t iDepthFromRoot = 0;
		while (iCurrent >= 0)
		{
			if (iCurrent == iRootBoneIndex)
			{
				m_UpperBodyMask[i] = std::min(
					static_cast<_float>(iDepthFromRoot + 1) / static_cast<_float>(iSafeBlendDepth),
					1.f);
				break;
			}

			iCurrent = Bones[iCurrent]->GetParendBoneIndex();
			++iDepthFromRoot;
		}
	}

	return true;
}

void CComAnimator::Play_Action(int32_t iActionIndex, _float fBlendDuration)
{
	if (m_pModelInstance == nullptr)
		return;

	auto pModel = m_pModelInstance->GetModel();
	if (pModel == nullptr)
		return;

	auto& ResAnims = pModel->GetAnimations();

	// Action 인덱스 검사
	if (iActionIndex < 0 || iActionIndex >= static_cast<int32_t>(m_Actions.size()))
		return;
	if (m_Actions[iActionIndex].Anims.empty())
		return;
	if (m_Actions[iActionIndex].Anims.front().iAnimIndex < 0 ||
		m_Actions[iActionIndex].Anims.front().iAnimIndex >= static_cast<int32_t>(ResAnims.size()))
		return;

	m_curActionsAnim = 0;
	m_curActions = iActionIndex;
	m_ActionTime = 0.f;
	// 같은 애니메이션이면 다시 세팅하지 않음
	if (m_CurAnimState.iAnimIndex == m_Actions[m_curActions].Anims[m_curActionsAnim].iAnimIndex)
		return;

	// 이전 상태 저장
	m_PrevAnimState = m_CurAnimState;

	// 현재 상태 새로 세팅
	m_CurAnimState.Reset();

	m_CurAnimState.iAnimIndex = m_Actions[m_curActions].Anims[m_curActionsAnim].iAnimIndex;
	m_CurAnimState.fTrackPosition = 0.f;
	m_CurAnimState.fSpeed = m_Actions[m_curActions].Anims[m_curActionsAnim].fSpeed;
	m_CurAnimState.bLoop = false;
	m_CurAnimState.bFinished = false;

	// 해당 애니메이션 채널 수만큼 키프레임 인덱스 준비
	m_CurAnimState.KeyFrameIndices.resize(ResAnims[m_CurAnimState.iAnimIndex]->GetNumChannel(), 0);

	// 블렌딩 시작 여부
	if (m_PrevAnimState.iAnimIndex >= 0 && fBlendDuration > 0.f)
	{
		m_bBlending = true                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                ;
		m_fBlendTime = 0.f;
		m_fBlendDuration = fBlendDuration;

		// 복사 
		m_BlendStartLocalMatrices = m_LocalBoneMatrices;
	}
	else
	{
		m_bBlending = false;
		m_fBlendTime = 0.f;
		m_fBlendDuration = 0.f;

	}

	m_iPlayAnimationType = ANIMTYPE::ACTION;
	m_bPlay = true;
	Invalidate_RootMotionCache();

}

void CComAnimator::Update_AnimState(_float fTimeDelta, ANIMSTRUCT& AnimState)
{
	if (m_pModelInstance == nullptr)
		return;

	auto pModel = m_pModelInstance->GetModel();
	if (pModel == nullptr)
		return;

	auto& Anims = pModel->GetAnimations();

	if (AnimState.iAnimIndex < 0 || AnimState.iAnimIndex >= static_cast<int32_t>(Anims.size()))
	{
		return;
	}

	auto pAnim = Anims[AnimState.iAnimIndex];
	if (pAnim == nullptr)
		return;

	_float fPrevTrackPosition = AnimState.fTrackPosition;

	AnimState.fTrackPosition += pAnim->GetTickPerSecond() * fTimeDelta * AnimState.fSpeed;

	const _float fDuration = pAnim->GetDuration();

	if (fDuration < 0.f)
		return;

	if (AnimState.fTrackPosition >= fDuration)
	{
		if (AnimState.bLoop)
		{
			AnimState.fTrackPosition = fmodf(AnimState.fTrackPosition, fDuration);

			// 루프가 발생했으므로 채널별 키프레임 인덱스 초기화
			std::fill(AnimState.KeyFrameIndices.begin(),AnimState.KeyFrameIndices.end(),0);
		}
		else
		{
			AnimState.fTrackPosition = fDuration;
			AnimState.bFinished = true;
		}
	}
	else if (AnimState.fTrackPosition < fPrevTrackPosition)
	{
		// 혹시 외부에서 TrackPosition을 되감았을 경우 방어
		std::fill(AnimState.KeyFrameIndices.begin(),AnimState.KeyFrameIndices.end(),0);
	}
}

void CComAnimator::Update_ActionState(_float fTimeDelta, ANIMSTRUCT& AnimState)
{
	if (m_pModelInstance == nullptr)
		return;

	auto pModel = m_pModelInstance->GetModel();
	if (pModel == nullptr)
		return;

	if (m_Actions.size() == 0 || m_Actions[0].Anims.size() == 0)
		return;

	auto& ResAnims = pModel->GetAnimations();

	if (AnimState.iAnimIndex < 0 || AnimState.iAnimIndex >= static_cast<int32_t>(ResAnims.size()))
	{
		return;
	}

	auto pResAnim = ResAnims[AnimState.iAnimIndex];
	if (pResAnim == nullptr)
		return;

	_float fPrevTrackPosition = AnimState.fTrackPosition;

	AnimState.fTrackPosition += pResAnim->GetTickPerSecond() * fTimeDelta * AnimState.fSpeed;
	m_ActionTime += pResAnim->GetTickPerSecond() * fTimeDelta * AnimState.fSpeed;
	

	const _float fDuration = pResAnim->GetDuration();

	if (fDuration <= 0.f)
		return;

	if (AnimState.fTrackPosition >= fDuration)
	{
		AnimState.fTrackPosition = fDuration;
		AnimState.bFinished = true;
		if (m_curActionsAnim < 0 || m_curActions < 0) {
			m_curActionsAnim = 0;
			m_iPlayAnimationType = ANIMTYPE::ACTION;
			m_curActions = 0;
			m_ActionTime = 0.f;
			m_bPlay = false;
		}
		else if (m_curActionsAnim + 1 >= static_cast<int32_t>(m_Actions[m_curActions].Anims.size())) {
			m_curActionsAnim = 0;			
			m_iPlayAnimationType = ANIMTYPE::ACTION;
			m_curActions = 0;
			m_ActionTime = 0.f;
			m_bPlay = false;
		}
		else {
			{
				m_curActionsAnim += 1;
				if (m_Actions[m_curActions].Anims[m_curActionsAnim].iAnimIndex < 0 ||
					m_Actions[m_curActions].Anims[m_curActionsAnim].iAnimIndex >= static_cast<int32_t>(ResAnims.size()))
				{
					m_bPlay = false;
					return;
				}

				// 이전 상태 저장
				m_PrevAnimState = m_CurAnimState;

				// 현재 상태 새로 세팅
				m_CurAnimState.Reset();

				m_CurAnimState.iAnimIndex = m_Actions[m_curActions].Anims[m_curActionsAnim].iAnimIndex;
				m_CurAnimState.fTrackPosition = 0.f;
				m_CurAnimState.fSpeed = m_Actions[m_curActions].Anims[m_curActionsAnim].fSpeed;
				m_CurAnimState.bLoop = false;
				m_CurAnimState.bFinished = false;

				// 해당 애니메이션 채널 수만큼 키프레임 인덱스 준비
				m_CurAnimState.KeyFrameIndices.resize(ResAnims[m_CurAnimState.iAnimIndex]->GetNumChannel(), 0);

				// 블렌딩 시작 여부
				if (m_PrevAnimState.iAnimIndex >= 0 && 0.2f > 0.f)
				{
					m_bBlending = true;
					m_fBlendTime = 0.f;
					m_fBlendDuration = 0.2f;

					// 복사 
					m_BlendStartLocalMatrices = m_LocalBoneMatrices;
				}
				else
				{
					m_bBlending = false;
					m_fBlendTime = 0.f;
					m_fBlendDuration = 0.f;

				}

				m_iPlayAnimationType = ANIMTYPE::ACTION;
				m_bPlay = true;
			}
		}
		
		
	}
	else if (AnimState.fTrackPosition < fPrevTrackPosition)
	{
		// 혹시 외부에서 TrackPosition을 되감았을 경우 방어
		std::fill(AnimState.KeyFrameIndices.begin(), AnimState.KeyFrameIndices.end(), 0);
	}
}

void CComAnimator::Build_BoneMatrices_CPU(_float fTimeDelta)
{
	if (m_pModelInstance == nullptr)
		return;

	auto pModel = m_pModelInstance->GetModel();
	if (pModel == nullptr)
		return;

	auto& Bones = pModel->GetBones();
	auto& Anims = pModel->GetAnimations();
	auto& m_CombinedBoneMatrices = m_pModelInstance->Get_CombinedBoneMatrices();
	if (m_CurAnimState.iAnimIndex < 0 ||
		m_CurAnimState.iAnimIndex >= static_cast<int32_t>(Anims.size()))
	{
		return;
	}

	auto pAnim = Anims[m_CurAnimState.iAnimIndex];
	if (pAnim == nullptr)
		return;

	const size_t iBoneCount = Bones.size();

	if (m_LocalBoneMatrices.size() != iBoneCount)
		m_LocalBoneMatrices.resize(iBoneCount);

	if (m_CombinedBoneMatrices.size() != iBoneCount)
		m_CombinedBoneMatrices.resize(iBoneCount);
	if (m_UpperLocalBoneMatrices.size() != iBoneCount)
		m_UpperLocalBoneMatrices.resize(iBoneCount);
	if (m_FinalLocalBoneMatrices.size() != iBoneCount)
		m_FinalLocalBoneMatrices.resize(iBoneCount);
	if (m_UpperBodyMask.size() != iBoneCount)
		m_UpperBodyMask.assign(iBoneCount, 0.f);


	// 1. 기본 Bone Local Matrix로 초기화
	for (size_t i = 0; i < iBoneCount; ++i)
	{
		XMStoreFloat4x4(&m_LocalBoneMatrices[i],Bones[i]->Get_TransformationMatrix());
	}

	const auto& Channels = pAnim->GetChannels();
	m_bRawRootLocalValid = false;

	if (m_CurAnimState.KeyFrameIndices.size() != Channels.size())
		m_CurAnimState.KeyFrameIndices.resize(Channels.size(), 0);

	for (uint32_t i = 0; i < static_cast<uint32_t>(Channels.size()); ++i)
	{
		Sample_Channel_CPU(Channels[i].get(), m_CurAnimState.fTrackPosition, m_CurAnimState.KeyFrameIndices[i], m_LocalBoneMatrices);
	}


	// Combined 올리기 전 Blend 
	Blend_Anim(fTimeDelta);
	Update_UpperLayer(fTimeDelta);
	Build_UpperLocalPose();
	Compose_FinalLocalPose();

	// 3. Combined Matrix 계산
	_matrix matPreTransform = XMLoadFloat4x4(&pModel->Get_PreTransformMatrix());


	uint32_t iMaxBoneDepth = 0;
	for (size_t i = 0; i < iBoneCount; ++i)
	{

		int32_t iParentIndex = Bones[i]->GetParendBoneIndex();


		if (-1 == iParentIndex) {
			XMStoreFloat4x4(&m_CombinedBoneMatrices[i], XMLoadFloat4x4(&m_FinalLocalBoneMatrices[i]) * matPreTransform);
			continue;
		}


		XMStoreFloat4x4(&m_CombinedBoneMatrices[i], XMLoadFloat4x4(&m_FinalLocalBoneMatrices[i]) * XMLoadFloat4x4(&m_CombinedBoneMatrices[iParentIndex]));

	}

	if (m_bRootMotion && m_bRawRootLocalValid && m_iRootBoneIndex >= 0)
	{
		const int32_t iParentIndex =
			Bones[m_iRootBoneIndex]->GetParendBoneIndex();
		_matrix currentRootCombined =
			XMLoadFloat4x4(&m_RawRootLocalMatrix);

		if (iParentIndex >= 0)
		{
			currentRootCombined *=
				XMLoadFloat4x4(&m_CombinedBoneMatrices[iParentIndex]);
		}
		else
		{
			currentRootCombined *= matPreTransform;
		}

		Apply_RootMotionFromCombined(currentRootCombined);
	}

	
}

void CComAnimator::Update_UpperLayer(_float fTimeDelta)
{
	if (!m_UpperAnimState.IsValid())
		return;

	Update_AnimState(fTimeDelta, m_UpperAnimState);

	if (m_fUpperFadeDuration > 0.f)
	{
		m_fUpperFadeTime += fTimeDelta;
		const _float fRatio = std::clamp(m_fUpperFadeTime / m_fUpperFadeDuration, 0.f, 1.f);
		m_fUpperLayerWeight = m_fUpperFadeStartWeight +
			(m_fUpperFadeTargetWeight - m_fUpperFadeStartWeight) * fRatio;

		if (fRatio >= 1.f)
			m_fUpperFadeDuration = 0.f;
	}

	if (m_UpperAnimState.bFinished && !m_UpperAnimState.bLoop && m_fUpperFadeTargetWeight > 0.f)
		Stop_UpperAnim(0.1f);

	if (m_fUpperLayerWeight <= 0.f && m_fUpperFadeTargetWeight <= 0.f)
	{
		m_fUpperLayerWeight = 0.f;
		m_UpperAnimState.Reset();
	}
}

void CComAnimator::Build_UpperLocalPose()
{
	if (!m_UpperAnimState.IsValid() || m_fUpperLayerWeight <= 0.f ||
		m_pModelInstance == nullptr || m_pModelInstance->GetModel() == nullptr)
		return;

	auto pModel = m_pModelInstance->GetModel();
	const auto& Bones = pModel->GetBones();
	const auto& Anims = pModel->GetAnimations();
	if (m_UpperAnimState.iAnimIndex < 0 ||
		m_UpperAnimState.iAnimIndex >= static_cast<int32_t>(Anims.size()))
		return;

	auto pAnim = Anims[m_UpperAnimState.iAnimIndex];
	if (pAnim == nullptr)
		return;

	for (uint32_t i = 0; i < (uint32_t)(Bones.size()); ++i)
	{
		_matrix matLocal = Bones[i]->Get_TransformationMatrix();
		if (auto* pChannel = pAnim->GetChannelByBoneIndex(i))
			matLocal = Evaluate_ChannelMatrix_CPU(pChannel, m_UpperAnimState.fTrackPosition);
		XMStoreFloat4x4(&m_UpperLocalBoneMatrices[i], matLocal);
	}
}

void CComAnimator::Compose_FinalLocalPose()
{
	if (m_FinalLocalBoneMatrices.size() != m_LocalBoneMatrices.size())
		m_FinalLocalBoneMatrices.resize(m_LocalBoneMatrices.size());

	const _bool bUseUpperLayer =
		m_UpperAnimState.IsValid() &&
		m_fUpperLayerWeight > 0.f &&
		m_UpperBodyMask.size() == m_LocalBoneMatrices.size();

	if (!bUseUpperLayer)
	{
		m_FinalLocalBoneMatrices = m_LocalBoneMatrices;
		return;
	}

	for (uint32_t i = 0; i < static_cast<uint32_t>(m_LocalBoneMatrices.size()); ++i)
	{
		const _float fWeight = std::clamp(m_UpperBodyMask[i] * m_fUpperLayerWeight, 0.f, 1.f);
		if (fWeight <= 0.f)
		{
			m_FinalLocalBoneMatrices[i] = m_LocalBoneMatrices[i];
			continue;
		}

		_vector vBaseScale, qBaseRotation, vBaseTranslation;
		_vector vUpperScale, qUpperRotation, vUpperTranslation;
		if (!XMMatrixDecompose(&vBaseScale, &qBaseRotation, &vBaseTranslation, XMLoadFloat4x4(&m_LocalBoneMatrices[i])) ||
			!XMMatrixDecompose(&vUpperScale, &qUpperRotation, &vUpperTranslation, XMLoadFloat4x4(&m_UpperLocalBoneMatrices[i])))
		{
			m_FinalLocalBoneMatrices[i] = m_LocalBoneMatrices[i];
			continue;
		}

		const _vector vScale = XMVectorLerp(vBaseScale, vUpperScale, fWeight);
		const _vector qRotation = XMQuaternionSlerp(qBaseRotation, qUpperRotation, fWeight);
		const _vector vTranslation = XMVectorLerp(vBaseTranslation, vUpperTranslation, fWeight);
		XMStoreFloat4x4(
			&m_FinalLocalBoneMatrices[i],
			XMMatrixAffineTransformation(
				vScale,
				XMVectorSet(0.f, 0.f, 0.f, 1.f),
				qRotation,
				XMVectorSetW(vTranslation, 1.f)));
	}
}

_bool CComAnimator::Sample_CombinedBoneMatrices(int32_t iAnimIndex, _float fTrackPosition, const std::vector<uint32_t>& boneChain,_float4x4& outMatrix) const
{
	if (m_pModelInstance == nullptr)
		return false;

	auto pModel = m_pModelInstance->GetModel();
	if (pModel == nullptr)
		return false;

	const auto& Bones = pModel->GetBones();
	const auto& Anims = pModel->GetAnimations();
	if (iAnimIndex < 0 || iAnimIndex >= (int32_t)(Anims.size()) || Anims[iAnimIndex] == nullptr)
		return false;


	const auto& pAnim = Anims[iAnimIndex];
	fTrackPosition = std::clamp(fTrackPosition, 0.f, pAnim->GetDuration());
	if (boneChain.empty())
		return false;


	const _matrix preTransform =XMLoadFloat4x4(&pModel->Get_PreTransformMatrix());
	_matrix combined = preTransform;


	for (uint32_t boneIndex : boneChain)
	{
		if (boneIndex >= Bones.size())
			return false;

		_matrix local = Bones[boneIndex]->Get_TransformationMatrix();
		if (auto* pChannel = pAnim->GetChannelByBoneIndex(boneIndex))
			local = Evaluate_ChannelMatrix_CPU(pChannel, fTrackPosition);


		if (m_bRootMotion && static_cast<int32_t>(boneIndex) == m_iRootBoneIndex)
		{
			_vector vScale;
			_vector vRotation;
			_vector vTranslation;
			if (!XMMatrixDecompose(&vScale, &vRotation, &vTranslation, local))
				return false;

			local = XMMatrixAffineTransformation(
				vScale,
				XMVectorSet(0.f, 0.f, 0.f, 1.f),
				RemoveYRotation(vRotation),
				XMVectorZero());
		}

		combined = local * combined;

	}

	XMStoreFloat4x4(&outMatrix, combined);



	return true;
}

void CComAnimator::Sample_Channel_CPU( CResModelChanel* pChannel,_float fTrackPosition,uint32_t& iCurrentKeyFrameIndex,std::vector<_float4x4>& OutLocalBoneMatrices)
{
	if (pChannel == nullptr)
		return;

	const auto& KeyFrames = pChannel->Get_KeyFrames();

	if (KeyFrames.empty())
		return;

	const int32_t iBoneIndex = pChannel->Get_BoneIndex();
	if (iBoneIndex < 0 ||
		static_cast<size_t>(iBoneIndex) >= OutLocalBoneMatrices.size())
		return;

	const auto& LastKeyFrame = KeyFrames.back();

	_vector vScale;
	_vector vRotation;
	_vector vTranslation;

	if (KeyFrames.size() == 1 || fTrackPosition >= LastKeyFrame.fTrackPosition)
	{
		vScale = XMLoadFloat3(&LastKeyFrame.vScale);
		vRotation = XMLoadFloat4(&LastKeyFrame.vRotation);
		vTranslation = XMVectorSetW(XMLoadFloat3(&LastKeyFrame.vTranslation),1.f);
		iCurrentKeyFrameIndex =
			static_cast<uint32_t>(KeyFrames.size() - 1);
	}
	else
	{
		// 애니메이션이 루프되어 시간이 되감겼거나 캐시가 범위를 벗어난 경우
		// 첫 키프레임부터 다시 탐색한다.
		if (iCurrentKeyFrameIndex + 1 >= KeyFrames.size() ||
			fTrackPosition < KeyFrames[iCurrentKeyFrameIndex].fTrackPosition)
		{
			iCurrentKeyFrameIndex = 0;
		}

		while (iCurrentKeyFrameIndex + 1 < KeyFrames.size() &&
			fTrackPosition >= KeyFrames[iCurrentKeyFrameIndex + 1].fTrackPosition)
		{
			++iCurrentKeyFrameIndex;
		}

		const auto& CurKeyFrame = KeyFrames[iCurrentKeyFrameIndex];
		const uint32_t iNextKeyFrameIndex = std::min(
			iCurrentKeyFrameIndex + 1,
			static_cast<uint32_t>(KeyFrames.size() - 1));
		const auto& NextKeyFrame = KeyFrames[iNextKeyFrameIndex];

		const _float fTimeDelta =
			NextKeyFrame.fTrackPosition - CurKeyFrame.fTrackPosition;
		const _float fRatio = std::clamp(
			(fTrackPosition - CurKeyFrame.fTrackPosition) /
				std::max(fTimeDelta, 0.00001f),
			0.f,
			1.f);

		vScale = XMVectorLerp(
			XMLoadFloat3(&CurKeyFrame.vScale),
			XMLoadFloat3(&NextKeyFrame.vScale),
			fRatio
		);

		vRotation = XMQuaternionSlerp(
			XMLoadFloat4(&CurKeyFrame.vRotation),
			XMLoadFloat4(&NextKeyFrame.vRotation),
			fRatio
		);

		vTranslation = XMVectorSetW(
			XMVectorLerp(
				XMLoadFloat3(&CurKeyFrame.vTranslation),
				XMLoadFloat3(&NextKeyFrame.vTranslation),
				fRatio
			),
			1.f
		);
	}



	_matrix matRawLocal = XMMatrixAffineTransformation(
		vScale,
		XMVectorSet(0.f, 0.f, 0.f, 1.f),
		vRotation,
		vTranslation);

	if (m_bRootMotion && iBoneIndex == m_iRootBoneIndex)
	{
		XMStoreFloat4x4(&m_RawRootLocalMatrix, matRawLocal);
		m_bRawRootLocalValid = true;
		vTranslation = XMVectorZero();
		vRotation = RemoveYRotation(vRotation);
	}
	_matrix matLocal =
		XMMatrixAffineTransformation(
			vScale,
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			vRotation,
			vTranslation
		);
	XMStoreFloat4x4(&OutLocalBoneMatrices[iBoneIndex],matLocal);
}

_matrix CComAnimator::Evaluate_ChannelMatrix_CPU(CResModelChanel* pChannel, _float fTrackPosition) const {
	// 이 함수는 처음 Load 해 올때만 Root Bone Transform 빼오기 위해서 만든 함수
	if (pChannel->Get_KeyFrames().empty())
		return XMMatrixIdentity();

	if (pChannel->Get_KeyFrames().size() == 1) {

		const KEYFRAME& KeyFrame = pChannel->Get_KeyFrames()[0];

		_vector vScale = XMLoadFloat3(&KeyFrame.vScale);
		_vector vRotation = XMLoadFloat4(&KeyFrame.vRotation);
		_vector vTranslation = XMVectorSetW(XMLoadFloat3(&KeyFrame.vTranslation), 1.f);

		return XMMatrixAffineTransformation(vScale, XMVectorSet(0.f, 0.f, 0.f, 1.f), vRotation, vTranslation);
	}


	uint32_t iKeyFrameIndex = pChannel->FindKeyFrameIndex(fTrackPosition);
	// 다음 프레임, 이전 프레임 Keyframe 
	const KEYFRAME& CurKeyFrame = pChannel->Get_KeyFrames()[iKeyFrameIndex];
	const KEYFRAME& NextKeyFrame = pChannel->Get_KeyFrames()[iKeyFrameIndex + 1];


	// 다음 프레임간의 Tickpersecond
	float fDuration = NextKeyFrame.fTrackPosition - CurKeyFrame.fTrackPosition;
	_float fRatio = 0.f;

	// 현재 프레임의 위치와 이전 프레임의 위치의 보간된 비율
	if (fDuration > 0.f)
	{
		fRatio = (fTrackPosition - CurKeyFrame.fTrackPosition) / fDuration;
	}



	_vector vScale = XMVectorLerp(
		XMLoadFloat3(&CurKeyFrame.vScale),
		XMLoadFloat3(&NextKeyFrame.vScale),
		fRatio
	);

	_vector vRotation = XMQuaternionSlerp(
		XMLoadFloat4(&CurKeyFrame.vRotation),
		XMLoadFloat4(&NextKeyFrame.vRotation),
		fRatio
	);

	_vector vTranslation = XMVectorSetW(
		XMVectorLerp(
			XMLoadFloat3(&CurKeyFrame.vTranslation),
			XMLoadFloat3(&NextKeyFrame.vTranslation),
			fRatio
		),
		1.f
	);

	// Lerp 함수로 보간 

	return XMMatrixAffineTransformation(vScale, XMVectorSet(0.f, 0.f, 0.f, 1.f), vRotation, vTranslation);

}

_matrix CComAnimator::Evaluate_RootCombinedMatrix_CPU(const CResModelAnim* pAnim,_float fTrackPosition) const
{
	if (!pAnim || !m_pModelInstance || m_iRootBoneIndex < 0)
		return XMMatrixIdentity();

	auto pModel = m_pModelInstance->GetModel();
	if (!pModel)
		return XMMatrixIdentity();

	const auto& Bones = pModel->GetBones();
	if ((size_t)(m_iRootBoneIndex) >= Bones.size())
		return XMMatrixIdentity();

	std::vector<uint32_t> boneChain;
	int32_t iBoneIndex = m_iRootBoneIndex;
	while (iBoneIndex >= 0 &&(size_t)(iBoneIndex) < Bones.size())
	{
		boneChain.push_back((uint32_t)(iBoneIndex));
		iBoneIndex = Bones[iBoneIndex]->GetParendBoneIndex();
	}
	std::reverse(boneChain.begin(), boneChain.end());

	_matrix combined = XMMatrixIdentity();
	for (uint32_t iChainBoneIndex : boneChain)
	{
		_matrix local = Bones[iChainBoneIndex]->Get_TransformationMatrix();
		if (auto* pChannel = pAnim->GetChannelByBoneIndex(iChainBoneIndex))
			local = Evaluate_ChannelMatrix_CPU(pChannel, fTrackPosition);

		combined = local * combined;
	}

	return combined * XMLoadFloat4x4(&pModel->Get_PreTransformMatrix());
}

void CComAnimator::Invalidate_RootMotionCache()
{
	m_bRawRootLocalValid = false;
	m_bPreviousRootCombinedValid = false;
	m_iRootMotionCacheAnimIndex = -1;
	m_vRootMotionDelta = _float3{ 0.f, 0.f, 0.f };
	m_qRootMotionRotationDelta = _float4{ 0.f, 0.f, 0.f, 1.f };
}

void CComAnimator::Prepare_RootMotionCache(
	const CResModelAnim* pAnim,
	_float fTrackPosition)
{
	if (!m_bRootMotion || m_iRootBoneIndex < 0 || pAnim == nullptr)
		return;

	if (m_bPreviousRootCombinedValid &&
		m_iRootMotionCacheAnimIndex == m_CurAnimState.iAnimIndex)
	{
		return;
	}

	const _matrix previousRootCombined =
		Evaluate_RootCombinedMatrix_CPU(pAnim, fTrackPosition);
	XMStoreFloat4x4(
		&m_PreviousRootCombinedMatrix,
		previousRootCombined);
	m_bPreviousRootCombinedValid = true;
	m_iRootMotionCacheAnimIndex = m_CurAnimState.iAnimIndex;
}

void CComAnimator::Apply_RootMotionFromCombined(_fmatrix currentRootCombined)
{
	if (m_bPreviousRootCombinedValid &&
		m_iRootMotionCacheAnimIndex == m_CurAnimState.iAnimIndex)
	{
		const _matrix previousRootCombined =
			XMLoadFloat4x4(&m_PreviousRootCombinedMatrix);
		_float3 previousPosition{};
		_float3 currentPosition{};
		XMStoreFloat3(&previousPosition, previousRootCombined.r[3]);
		XMStoreFloat3(&currentPosition, currentRootCombined.r[3]);

		m_vRootMotionDelta.x = currentPosition.x - previousPosition.x;
		m_vRootMotionDelta.y = 0.f;
		m_vRootMotionDelta.z = currentPosition.z - previousPosition.z;

		_vector previousScale;
		_vector previousRotation;
		_vector previousTranslation;
		_vector currentScale;
		_vector currentRotation;
		_vector currentTranslation;
		if (XMMatrixDecompose(
				&previousScale,
				&previousRotation,
				&previousTranslation,
				previousRootCombined) &&
			XMMatrixDecompose(
				&currentScale,
				&currentRotation,
				&currentTranslation,
				currentRootCombined))
		{
			const _vector deltaRotation = XMQuaternionNormalize(
				XMQuaternionMultiply(
					currentRotation,
					XMQuaternionInverse(previousRotation)));
			XMStoreFloat4(
				&m_qRootMotionRotationDelta,
				deltaRotation);
		}
	}

	XMStoreFloat4x4(
		&m_PreviousRootCombinedMatrix,
		currentRootCombined);
	m_bPreviousRootCombinedValid = true;
	m_iRootMotionCacheAnimIndex = m_CurAnimState.iAnimIndex;
}

void CComAnimator::Update_RootMotion_GPU(
	const CResModelAnim* pAnim,
	_float fPreviousTrackPosition)
{
	m_vRootMotionDelta = _float3{ 0.f, 0.f, 0.f };
	m_qRootMotionRotationDelta = _float4{ 0.f, 0.f, 0.f, 1.f };

	if (!m_bRootMotion || m_iRootBoneIndex < 0 || pAnim == nullptr ||
		pAnim->GetChannelByBoneIndex(m_iRootBoneIndex) == nullptr)
	{
		return;
	}

	// A loop wrap must not subtract the end pose from the beginning pose.
	if (m_CurAnimState.fTrackPosition < fPreviousTrackPosition)
	{
		Invalidate_RootMotionCache();
		Prepare_RootMotionCache(pAnim, m_CurAnimState.fTrackPosition);
		return;
	}

	Prepare_RootMotionCache(pAnim, fPreviousTrackPosition);
	const _matrix currentRootCombined = Evaluate_RootCombinedMatrix_CPU(
		pAnim,
		m_CurAnimState.fTrackPosition);
	Apply_RootMotionFromCombined(currentRootCombined);
}

_vector CComAnimator::RemoveYRotation(_vector qRotation) const
{
	qRotation = XMQuaternionNormalize(qRotation);

	// Quaternion의 Y축 회전 성분(Twist)만 추출
	_vector qTwistY = XMVectorSet(
		0.f,
		XMVectorGetY(qRotation),
		0.f,
		XMVectorGetW(qRotation)
	);

	const float fLengthSq =
		XMVectorGetX(XMVector4LengthSq(qTwistY));

	if (fLengthSq < 0.000001f)
		return qRotation;

	qTwistY = XMQuaternionNormalize(qTwistY);

	// 원본 회전에서 Y축 회전만 제거
	// DirectXMath의 Multiply는 두 번째 인자가 왼쪽에 곱해짐
	_vector qResult = XMQuaternionMultiply(
		XMQuaternionInverse(qTwistY),
		qRotation
	);

	return XMQuaternionNormalize(qResult);
}

void CComAnimator::Blend_Anim(_float fTimeDelta)
{
	if (!m_bBlending || m_BlendStartLocalMatrices.empty())
		return;


	m_fBlendTime += fTimeDelta;

    _float fRatio = m_fBlendDuration > 0.f ?
		m_fBlendTime / m_fBlendDuration : 1.f;

    fRatio = std::min(std::max(fRatio, 0.f), 1.f);

    for (size_t i = 0; i < m_LocalBoneMatrices.size(); ++i)
    {
        _matrix StartMatrix = XMLoadFloat4x4(&m_BlendStartLocalMatrices[i]);
        _matrix EndMatrix = XMLoadFloat4x4(&m_LocalBoneMatrices[i]);
      
        _vector StartScale, StartRotation, StartTranslation;
        _vector EndScale, EndRotation, EndTranslation;

        if (FALSE == XMMatrixDecompose(&StartScale, &StartRotation, &StartTranslation, StartMatrix) ||
            FALSE == XMMatrixDecompose(&EndScale, &EndRotation, &EndTranslation, EndMatrix))
        {
            continue;
        }

        _vector BlendScale = XMVectorLerp(StartScale, EndScale, fRatio);
        _vector BlendRotation = XMQuaternionSlerp(StartRotation, EndRotation, fRatio);
        _vector BlendTranslation = XMVectorLerp(StartTranslation, EndTranslation, fRatio);

        _matrix BlendMatrix = XMMatrixAffineTransformation(
            BlendScale,
            XMVectorSet(0.f, 0.f, 0.f, 1.f),
            BlendRotation,
            XMVectorSetW(BlendTranslation, 1.f));

		XMStoreFloat4x4(&m_LocalBoneMatrices[i], BlendMatrix);
		
    }

    if (fRatio >= 1.f)
    {
		m_bBlending = false;
		m_fBlendTime = 0.f;
		m_fBlendDuration = 0.f;
    }
}

void CComAnimator::Advance_GPUBlend(_float fTimeDelta)
{
	if (!m_bBlending)
		return;

	m_fBlendTime += fTimeDelta;
	if (GetBlendWeight() >= 1.f)
	{
		m_bBlending = false;
		m_fBlendTime = 0.f;
		m_fBlendDuration = 0.f;
	}
}

void CComAnimator::SetTrackPosition(
	_float fTrackPosition,
	_bool bPreserveBlend)
{
	if (m_pModelInstance == nullptr)
		return;

	auto pModel = m_pModelInstance->GetModel();
	if (pModel == nullptr)
		return;

	auto& Anims = pModel->GetAnimations();

	if (m_CurAnimState.iAnimIndex < 0 ||
		m_CurAnimState.iAnimIndex >= static_cast<int32_t>(Anims.size()))
	{
		return;
	}

	auto pAnim = Anims[m_CurAnimState.iAnimIndex];
	if (pAnim == nullptr)
		return;

	_float fDuration = pAnim->GetDuration();
	
	if (fDuration <= 0.f)
		return;

	m_CurAnimState.fDuration = fDuration;
	fTrackPosition = std::clamp(fTrackPosition, 0.f, fDuration);

	m_CurAnimState.fTrackPosition = fTrackPosition;
	Invalidate_RootMotionCache();

	std::fill(
		m_CurAnimState.KeyFrameIndices.begin(),
		m_CurAnimState.KeyFrameIndices.end(),
		0
	);


	if (!bPreserveBlend)
	{
		m_bBlending = false;
		m_fBlendTime = 0.f;
		m_fBlendDuration = 0.f;
	}


	Build_BoneMatrices_CPU(0.f);

	m_fRatio = m_CurAnimState.fTrackPosition / fDuration;
}



UPtr<CComAnimator> CComAnimator::Create()
{
    auto pInstance = ToUPtr(new CComAnimator{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComAnimator");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComAnimator::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComAnimator{ *this });
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned: CComAnimator");
        return nullptr;
    }
    return pInstance;
}
