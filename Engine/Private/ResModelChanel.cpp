#include "pch.h"
#include "ResModelChanel.h"


#include <fstream>

NS_USING(Engine)

CResModelChanel::CResModelChanel(const _string& sPath)
    : CResource{ sPath }
{
}

CResModelChanel::~CResModelChanel()
{
}

HRESULT CResModelChanel::Load(const std::any& arg)
{

    auto descArg = std::any_cast<DESC>(&arg);
    if (!descArg)
    {
        return E_FAIL;
    }

    if (m_eState == STATE::LOADED)
    {
        return S_OK;
    }
    m_eState = STATE::LOADING;
    auto& pModel = descArg->pModel;
    auto pPoint = descArg->ptr;
    {

        m_iBoneIndex = *(uint32_t*)pPoint;
        pPoint += sizeof(uint32_t);




        if (-1 == m_iBoneIndex)
            return E_FAIL;

        m_iNumKeyFrames = *(uint32_t*)pPoint;
        pPoint += sizeof(uint32_t);
		
        _float3     vScale = {};
        _float4     vRotation = {};
        _float3     vTranslation = {};
        m_KeyFrames.clear();
        m_KeyFrames.resize(m_iNumKeyFrames);
        

        

        for (uint32_t i = 0; i < m_iNumKeyFrames; ++i)
        {
            KEYFRAME& KeyFrame = m_KeyFrames[i];

            memcpy(&KeyFrame.vScale, pPoint, sizeof(XMFLOAT3));
            pPoint += sizeof(XMFLOAT3);

            memcpy(&KeyFrame.vRotation, pPoint, sizeof(XMFLOAT4));
            pPoint += sizeof(XMFLOAT4);

            memcpy(&KeyFrame.vTranslation, pPoint, sizeof(XMFLOAT3));
            pPoint += sizeof(XMFLOAT3);

            memcpy(&KeyFrame.fTrackPosition, pPoint, sizeof(float));
            pPoint += sizeof(float);
        }

    }

    m_eState = STATE::LOADED;
    return S_OK;
}

HRESULT CResModelChanel::Unload(const std::any& arg)
{

    m_eState = STATE::UNLOAD;
    return S_OK;
}

//void CResModelChanel::Update_TransformationMatrix(uint32_t& iCurrentKeyFrameIndex, _float fCurrentTrackPosition, const std::vector<SPtr<CResModelBone>>& Bones,int32_t m_iRootBoneIndex)
//{
//    if (0.f == fCurrentTrackPosition)
//        iCurrentKeyFrameIndex = 0;
//
//    KEYFRAME        LastKeyFrame = m_KeyFrames.back();
//
//    _vector         vScale, vRotation, vTranslation;
//
//    if (fCurrentTrackPosition >= LastKeyFrame.fTrackPosition)
//    {
//        vScale = XMLoadFloat3(&LastKeyFrame.vScale);
//        vRotation = XMLoadFloat4(&LastKeyFrame.vRotation);
//        vTranslation = XMVectorSetW(XMLoadFloat3(&LastKeyFrame.vTranslation), 1.f);
//    }
//    else
//    {
//        while (fCurrentTrackPosition >= m_KeyFrames[iCurrentKeyFrameIndex + 1].fTrackPosition)
//            ++iCurrentKeyFrameIndex;
//
//        _float      fRatio = (fCurrentTrackPosition - m_KeyFrames[iCurrentKeyFrameIndex].fTrackPosition) /
//            (m_KeyFrames[iCurrentKeyFrameIndex + 1].fTrackPosition - m_KeyFrames[iCurrentKeyFrameIndex].fTrackPosition);
//
//        vScale = XMVectorLerp(
//            XMLoadFloat3(&m_KeyFrames[iCurrentKeyFrameIndex].vScale),
//            XMLoadFloat3(&m_KeyFrames[iCurrentKeyFrameIndex + 1].vScale),
//            fRatio
//        );
//
//        vRotation = XMQuaternionSlerp(
//            XMLoadFloat4(&m_KeyFrames[iCurrentKeyFrameIndex].vRotation),
//            XMLoadFloat4(&m_KeyFrames[iCurrentKeyFrameIndex + 1].vRotation),
//            fRatio
//        );
//
//        vTranslation = XMVectorSetW(XMVectorLerp(
//            XMLoadFloat3(&m_KeyFrames[iCurrentKeyFrameIndex].vTranslation),
//            XMLoadFloat3(&m_KeyFrames[iCurrentKeyFrameIndex + 1].vTranslation),
//            fRatio
//        ), 1.f);
//    }
//
//    _matrix         TransformationMatrix = XMMatrixAffineTransformation(vScale, XMVectorSet(0.f, 0.f, 0.f, 1.f), vRotation, vTranslation);
//
//	// 해당되는 RootBone이 들어오면 이동량 삭제
//	if (m_iBoneIndex == m_iRootBoneIndex)
//	{
//		TransformationMatrix.r[3] = XMVectorSet(0.f, 0.f, 0.f, 1.f);
//	}
//
//    Bones[m_iBoneIndex]->Set_TransformationMatrix(TransformationMatrix);
//}

uint32_t CResModelChanel::FindKeyFrameIndex(float fTrackPos)const
{
    if (m_KeyFrames.size() < 2)
        return 0;

    for (uint32_t i = 0; i < m_KeyFrames.size() - 1; ++i)
    {
        if (fTrackPos < m_KeyFrames[i + 1].fTrackPosition)
            return i;
    }

    return static_cast<uint32_t>(m_KeyFrames.size() - 2);
}

_matrix CResModelChanel::Evaluate_TransformationMatrix(_float fTrackPosition) const {
	// 이 함수는 처음 Load 해 올때만 Root Bone Transform 빼오기 위해서 만든 함수
	if (m_KeyFrames.empty())
		return XMMatrixIdentity();

	if (m_KeyFrames.size() == 1) {

		const KEYFRAME& KeyFrame = m_KeyFrames[0];

		_vector vScale = XMLoadFloat3(&KeyFrame.vScale);
		_vector vRotation = XMLoadFloat4(&KeyFrame.vRotation);
		_vector vTranslation = XMVectorSetW(XMLoadFloat3(&KeyFrame.vTranslation), 1.f);

		return XMMatrixAffineTransformation(vScale,XMVectorSet(0.f, 0.f, 0.f, 1.f),vRotation,vTranslation);
	}


	uint32_t iKeyFrameIndex = FindKeyFrameIndex(fTrackPosition);
	// 다음 프레임, 이전 프레임 Keyframe 
	const KEYFRAME& CurKeyFrame = m_KeyFrames[iKeyFrameIndex];
	const KEYFRAME& NextKeyFrame = m_KeyFrames[iKeyFrameIndex + 1];


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

	return XMMatrixAffineTransformation(vScale,XMVectorSet(0.f, 0.f, 0.f, 1.f),vRotation,vTranslation);

}


SPtr<CResModelChanel> CResModelChanel::Create(const _string& sPath)
{
    return ToSPtr(new CResModelChanel{ sPath });
}
