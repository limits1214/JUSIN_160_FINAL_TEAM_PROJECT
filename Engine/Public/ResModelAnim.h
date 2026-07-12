#pragma once

#include "Resource.h"
struct aiAnimation;


NS_BEGIN(Engine)


class CResModel;

struct CChannel
{
	int32_t				m_iBoneIndex = {};
	uint32_t			m_iNumKeyFrames = {};
	std::vector<KEYFRAME>	m_KeyFrames;
	std::vector<KEYFRAME>   m_RootKeyFrames;

	int32_t				Get_BoneIndex() { return m_iBoneIndex; }
	uint32_t			Get_NumKeyFrames() { return m_iNumKeyFrames; }
	std::vector<KEYFRAME>& Get_KeyFrames() { return m_KeyFrames; }
	uint32_t FindKeyFrameIndex(float fTrackPos)const;
	_matrix Evaluate_TransformationMatrix(_float fTrackPosition) const;
	HRESULT Intialize(_char* ptr, CResModel* pModel);
};
inline  HRESULT CChannel::Intialize(_char* ptr, CResModel* pModel)
{
	auto pPoint = ptr;
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
	return S_OK;
}
inline  uint32_t CChannel::FindKeyFrameIndex(float fTrackPos)const
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
inline  _matrix CChannel::Evaluate_TransformationMatrix(_float fTrackPosition) const
{
	if (m_KeyFrames.empty())
		return XMMatrixIdentity();

	if (m_KeyFrames.size() == 1) {

		const KEYFRAME& KeyFrame = m_KeyFrames[0];

		_vector vScale = XMLoadFloat3(&KeyFrame.vScale);
		_vector vRotation = XMLoadFloat4(&KeyFrame.vRotation);
		_vector vTranslation = XMVectorSetW(XMLoadFloat3(&KeyFrame.vTranslation), 1.f);

		return XMMatrixAffineTransformation(vScale, XMVectorSet(0.f, 0.f, 0.f, 1.f), vRotation, vTranslation);
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

	return XMMatrixAffineTransformation(vScale, XMVectorSet(0.f, 0.f, 0.f, 1.f), vRotation, vTranslation);
}

class CResModelChanel;
class CResModel;
class ENGINE_DLL CResModelAnim final : public CResource
{
public:
	DECLARE_DERIVED_TYPE(CResModelAnim, CResource)
public:
	typedef struct tagDesc {
		CResModel* pModel;
		std::string& path;
	}DESC;
private:
	explicit CResModelAnim(const _string& sPath);
	~CResModelAnim() override;

public:
	HRESULT Load(const std::any& arg = {}) override;
	HRESULT Unload(const std::any& arg = {}) override;

	//_bool Update_TransformationMatrices(_float fTimeDelta, const std::vector<SPtr<CResModelBone>>& Bones, _bool isLoop);
	_bool ExtractRootMotionDelta(_float fPrevTrackPosition, _float fCurrTrackPosition, uint32_t iRootBoneIndex, _float3& vOutDelta);
	CChannel* FindRootChannel(uint32_t iRootBoneIndex);
	void RebuildCurrentKeyFrameIndices();

public:
	_float  GetDuration() const { return m_fDuration; }
	_float  GetTickPerSecond() const { return m_fTickPerSecond; }
	_float  GetCurrentTrackPosition() const { return m_fCurrentTrackPosition; }

	void    SetDuration(_float fDuration) { m_fDuration = fDuration; }
	void    SetTickPerSecond(_float fTickPerSecond) { m_fTickPerSecond = fTickPerSecond; }
	void    SetCurrentTrackPosition(_float fCurrentTrackPosition) ;

	std::string& GetAnimName() { return m_AnimName; }
	void		 SetAnimName(std::string _name) { m_AnimName = _name; }

	std::string& GetAnimPath() { return m_AnimPath; }
	void	 SetAnimPath(std::string _path) { m_AnimPath = _path; }	

	uint32_t	GetNumChannel() { return m_iNumChannels; };
	std::vector<CChannel>& GetChannels() { return m_Channels; }


	int32_t     GetRootBoneIndex() { return m_iRootBoneIndex; }

private:
	// 런 타임 도중만 가지는 주소
	std::string			m_AnimPath;
	std::string			m_AnimName;

	/* 이 애니메이션의 총 길이. */
	_float				m_fDuration = {};
	_float				m_fTickPerSecond = {};
	_float				m_fCurrentTrackPosition = {};

	/* 컨트롤해야하는 뼈의 갯수 */
	uint32_t							m_iNumChannels = {};
	//std::vector<SPtr<CResModelChanel>>	m_Channels;
	std::vector<CChannel> m_Channels;
	std::vector<uint32_t>					m_CurrentKeyFrameIndices;

	int32_t								m_iRootBoneIndex{};
	_float3								m_vRootDelta;

public:

	static SPtr<CResModelAnim> Create(const _string& sPath = {});
};

NS_END
