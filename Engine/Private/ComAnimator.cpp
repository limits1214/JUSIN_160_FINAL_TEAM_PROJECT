#include "pch.h"
#include "GameInstance.h"
#include "ComAnimator.h"
#include "ComModelInstance.h"
#include "ResModelAnim.h"
#include "ResModel.h"
NS_USING(Engine)



CComAnimator::CComAnimator()
{


}

CComAnimator::~CComAnimator()
{
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


		m_iRootBoneIndex = m_pModelInstance->GetModel()->Get_BoneIndex("Reference");

    }


    return S_OK;
}

HRESULT CComAnimator::Update(_float fTimeDelta)
{
    if (m_bPlay) {
        switch (m_iPlayAnimationType) {
        case ANIMTYPE::MONTAGE: {
			if (m_iPlayAnimMonatgueIndex < 0)
				break;
			
        }break;
        case ANIMTYPE::ANIM: {
				
			Update_Anim(fTimeDelta);
        }
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

	if (m_bRootMotion && m_iRootBoneIndex >= 0)
	{
		auto pRootChannel = pAnim->FindRootChannel(m_iRootBoneIndex);

		if (pRootChannel)
		{
			_matrix matPrev = Evaluate_ChannelMatrix_CPU(*pRootChannel, fPrevTrackPosition);
			_matrix matCurr = Evaluate_ChannelMatrix_CPU(*pRootChannel, m_CurAnimState.fTrackPosition);

			_float3 vPrevPos{};
			_float3 vCurrPos{};

			XMStoreFloat3(&vPrevPos, matPrev.r[3]);
			XMStoreFloat3(&vCurrPos, matCurr.r[3]);

			m_vRootMotionDelta.x = vCurrPos.x - vPrevPos.x;
			m_vRootMotionDelta.y = 0.f;
			m_vRootMotionDelta.z = vCurrPos.z - vPrevPos.z;
		}
	}

	// 계산 CPU에서 GPU로 넘어가는건 안정화다음 작업 최적화 할떄 고치기


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
	m_CurAnimState.KeyFrameIndices.resize(
		Anims[iAnimIndex]->GetNumChannel(),
		0
	);

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

	m_iPlayAnimationType = ANIMTYPE::ANIM;
	m_bPlay = true;
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

	if (fDuration <= 0.f)
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


	// 1. 기본 Bone Local Matrix로 초기화
	for (size_t i = 0; i < iBoneCount; ++i)
	{
		XMStoreFloat4x4(&m_LocalBoneMatrices[i],Bones[i]->Get_TransformationMatrix());
	}

	 auto& Channels = pAnim->GetChannels();

	if (m_CurAnimState.KeyFrameIndices.size() != Channels.size())
		m_CurAnimState.KeyFrameIndices.resize(Channels.size(), 0);

	for (uint32_t i = 0; i < static_cast<uint32_t>(Channels.size()); ++i)
	{
		Sample_Channel_CPU(Channels[i], m_CurAnimState.fTrackPosition, m_CurAnimState.KeyFrameIndices[i], m_LocalBoneMatrices);
	}


	// Combined 올리기 전 Blend 
	Blend_Anim(fTimeDelta);

	// 3. Combined Matrix 계산
	_matrix matPreTransform = XMLoadFloat4x4(&pModel->Get_PreTransformMatrix());


	for (size_t i = 0; i < iBoneCount; ++i)
	{

		int32_t iParentIndex = Bones[i]->GetParendBoneIndex();


		if (-1 == iParentIndex) {
			XMStoreFloat4x4(&m_CombinedBoneMatrices[i], XMLoadFloat4x4(&m_LocalBoneMatrices[i]) * matPreTransform);
			continue;
		}


		XMStoreFloat4x4(&m_CombinedBoneMatrices[i], XMLoadFloat4x4(&m_LocalBoneMatrices[i]) * XMLoadFloat4x4(&m_CombinedBoneMatrices[iParentIndex]));

	}

	
}

void CComAnimator::Sample_Channel_CPU(CChannel& pChannel,_float fTrackPosition,uint32_t& iCurrentKeyFrameIndex,std::vector<_float4x4>& OutLocalBoneMatrices)
{
	//if (pChannel == nullptr)
	//	return;

	const auto& KeyFrames = pChannel.Get_KeyFrames();

	if (KeyFrames.empty())
		return;

	const int32_t iBoneIndex = pChannel.Get_BoneIndex();



	if (fTrackPosition == 0.f)
		iCurrentKeyFrameIndex = 0;

	const auto& LastKeyFrame = KeyFrames.back();

	_vector vScale;
	_vector vRotation;
	_vector vTranslation;

	if (fTrackPosition >= LastKeyFrame.fTrackPosition)
	{
		vScale = XMLoadFloat3(&LastKeyFrame.vScale);
		vRotation = XMLoadFloat4(&LastKeyFrame.vRotation);
		vTranslation = XMVectorSetW(XMLoadFloat3(&LastKeyFrame.vTranslation),1.f);
	}
	else
	{
		while (iCurrentKeyFrameIndex + 1 < KeyFrames.size() &&
			fTrackPosition >= KeyFrames[iCurrentKeyFrameIndex + 1].fTrackPosition)
		{
			++iCurrentKeyFrameIndex;
		}

		const auto& CurKeyFrame = KeyFrames[iCurrentKeyFrameIndex];
		const auto& NextKeyFrame = KeyFrames[iCurrentKeyFrameIndex + 1];

		_float fRatio =
			(fTrackPosition - CurKeyFrame.fTrackPosition) /
			(NextKeyFrame.fTrackPosition - CurKeyFrame.fTrackPosition);

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

	_matrix matLocal =
		XMMatrixAffineTransformation(
			vScale,
			XMVectorSet(0.f, 0.f, 0.f, 1.f),
			vRotation,
			vTranslation
		);

	if (m_bRootMotion && iBoneIndex == m_iRootBoneIndex)
	{
		matLocal.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);
	}

	XMStoreFloat4x4(&OutLocalBoneMatrices[iBoneIndex],matLocal);
}

_matrix CComAnimator::Evaluate_ChannelMatrix_CPU(CChannel& pChannel, _float fTrackPosition)  {
	// 이 함수는 처음 Load 해 올때만 Root Bone Transform 빼오기 위해서 만든 함수
	if (pChannel.Get_KeyFrames().empty())
		return XMMatrixIdentity();

	if (pChannel.Get_KeyFrames().size() == 1) {

		const KEYFRAME& KeyFrame = pChannel.Get_KeyFrames()[0];

		_vector vScale = XMLoadFloat3(&KeyFrame.vScale);
		_vector vRotation = XMLoadFloat4(&KeyFrame.vRotation);
		_vector vTranslation = XMVectorSetW(XMLoadFloat3(&KeyFrame.vTranslation), 1.f);

		return XMMatrixAffineTransformation(vScale, XMVectorSet(0.f, 0.f, 0.f, 1.f), vRotation, vTranslation);
	}


	uint32_t iKeyFrameIndex = pChannel.FindKeyFrameIndex(fTrackPosition);
	// 다음 프레임, 이전 프레임 Keyframe 
	const KEYFRAME& CurKeyFrame = pChannel.Get_KeyFrames()[iKeyFrameIndex];
	const KEYFRAME& NextKeyFrame = pChannel.Get_KeyFrames()[iKeyFrameIndex + 1];


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

void CComAnimator::Blend_Anim(_float fTimeDelta)
{
	if (!m_bBlending)
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

HRESULT CComAnimator::AnimEditor_Play_AnimResource(_float fTimeDelta, uint32_t iModelAnimNum)
{
	auto pModel = GetGameObject()->GetComponent<CComModelInstance>(m_Comtag)->GetModel();
    
    if(pModel == nullptr)
		return E_FAIL;

    auto& pAnim = pModel->GetAnimations();
    auto& m_PreTransformMatrix = pModel->Get_PreTransformMatrix();

    _bool           isFinished = { false };

    /* 뼈들의 m_TransformationMatrix를 갱신해준다. */
    //isFinished = pAnim[iModelAnimNum]->Update_TransformationMatrices(fTimeDelta, pModel->GetBones(), m_bLoop);

    //for (auto& pBone : pModel->GetBones())
    //{
    //    pBone->Update_CombinedTransformationMatrix(pModel->GetBones(), XMLoadFloat4x4(&m_PreTransformMatrix));
    //}

    return isFinished;
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
