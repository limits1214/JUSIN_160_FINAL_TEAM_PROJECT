
#pragma once

#include "Component.h"

NS_BEGIN(Engine)
class CComModelInstance;
class CComAnimMontage;
class CResModelAnim;


class ENGINE_DLL CComAnimator : public CComponent
{


public:
	typedef struct tagDesc : CComponent::DESC
	{
		std::string_view  sComTag;
	
	}DESC;

public:
	enum ANIMTYPE{
		ANIM, MONTAGE
	};

	struct ANIMSTRUCT {
		int32_t   iAnimIndex = -1;

		// 재생 시간
		_float	  fTrackPosition = 0.f;
		// 재생 속도
		_float    fSpeed = 1.f;
		// 재생 루프
		_bool	bLoop = true;
		_bool	bFinished = false;

		// 키 몇개 있는지
		std::vector<uint32_t>	KeyFrameIndices;

		void Reset() {
			iAnimIndex = -1;
			fTrackPosition = 0.f;
			fSpeed = 1.f;
			bLoop = true;
			bFinished = false;
			KeyFrameIndices.clear();
		}

		void SetAnim(int32_t iIndex, _bool isLoop = true, _float fAnimSpeed = 1.f)
		{
			iAnimIndex = iIndex;
			fTrackPosition = 0.f;
			fSpeed = fAnimSpeed;
			bLoop = isLoop;
			bFinished = false;
			KeyFrameIndices.clear();
		}

		_bool IsValid() const
		{
			return iAnimIndex >= 0;
		}

	};
	 

public:
	DECLARE_DERIVED_TYPE(CComAnimator, CComponent)


private:
	explicit CComAnimator();
	~CComAnimator() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	HRESULT Update(_float fTimeDelta);


	// AnimUPdate
	HRESULT	Update_Anim(_float fTimeDelta);
	void	Play_Anim(int32_t iAnimIndex, _bool bLoop=false, _float fBlendDuration = 0.1f);
	void	Update_AnimState(_float fTimeDelta, ANIMSTRUCT& AnimState);
	void	Build_BoneMatrices_CPU(_float fTimeDelta);
	void	Sample_Channel_CPU( CChannel& pChannel, _float fTrackPosition, uint32_t& iCurrentKeyFrameIndex, std::vector<_float4x4>& OutLocalBoneMatrices);
	_matrix Evaluate_ChannelMatrix_CPU(CChannel& pChannel, _float fTrackPosition);


	void	Blend_Anim(_float fTimeDelta);


public:
	_float3 GetRootMotionDelta() { return m_vRootMotionDelta; }

private:
	CComModelInstance* m_pModelInstance;
	std::unordered_map<int32_t, CComAnimMontage*> m_mapAnimMontages;
	int32_t		   m_iAnimMontageIndex{ -1 };

private:
	_string			m_Comtag;
	ANIMTYPE		m_iPlayAnimationType{ ANIMTYPE::ANIM };
	int32_t		m_iPlayAnimationNum{ 0 };
	int32_t		m_iPlayAnimIndex{ 0 };
	int32_t		m_iPlayAnimMonatgueIndex{ 0 };

private:
	// 애니메이션 정보 (블렌딩 고려)
	ANIMSTRUCT m_CurAnimState;
	ANIMSTRUCT m_PrevAnimState;

	// 애니메이션 제어를 위한 멤버 변수들
	_bool		    m_bPlay{ false };
	_bool			m_bLoop{ false };
	_bool		    m_bFinish{ false };
	_bool			m_bBlending = true;
	_float          m_fRatio{ 0.f };
	_float			m_fBlendTime = 0.f;
	_float			m_fBlendDuration;

	/* 이 애니메이션의 총 길이. */
	_float				m_fDuration = {};
	_float				m_fTickPerSecond = {};
	_float				m_fCurrentTrackPosition = {};

	std::vector<_float4x4>				m_LocalBoneMatrices;
	std::vector<_float4x4>				m_BlendStartLocalMatrices;
private:
	_bool   m_bRootMotion{ true };
	int32_t m_iRootBoneIndex{ -1 };
	// 애니메이션 Local 기준 RootMotion
	_float3 m_vRootMotionDelta{ 0.f, 0.f, 0.f };

public:
	static UPtr<CComAnimator> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

	_bool GetFinish() const { return m_bFinish; }

	_bool GetPlay() const { return m_bPlay; }
	void  SetPlay(_bool bPlay) { m_bPlay = bPlay;}

	_bool GetLoop() const { return m_bLoop; }
	void  SetLoop(_bool _bLoop) { m_bLoop = _bLoop; }



	// 다 지울것들
public:

public:
	HRESULT AnimEditor_Play_AnimResource(_float fTimeDelta, uint32_t iModelAnimNum);

	std::unordered_map<int32_t, CComAnimMontage*>& GetAnimMontages() { return m_mapAnimMontages; }

	int32_t Get_CurrAnimMontageIndex() { return m_iAnimMontageIndex; }
	void SetCurrentAnimMontageIndex(int32_t iIndex) { m_iAnimMontageIndex = iIndex; }
public:
	uint32_t GetAnimationTYPE() const { return ETOUI(m_iPlayAnimationType); }
	void SetAnimationTYPE(ANIMTYPE eType) { m_iPlayAnimationType = eType; }	

	uint32_t GetPlayAnimIndex() const { return m_iPlayAnimIndex; }
	void SetPlayAnimIndex(uint32_t iIndex) { m_iPlayAnimIndex = iIndex; }

	_float GetPlayAnimRatio() const { return m_fRatio; }

};

NS_END
