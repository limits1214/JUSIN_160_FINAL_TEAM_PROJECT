
#pragma once

#include "Component.h"

#include "ISerializable.h"
#include "SerializerInterface.h"

NS_BEGIN(Engine)
class CComModelInstance;
class CResModelAnim;


class ENGINE_DLL CComAnimator : public CComponent
{

public:
	void UpdateGUI() override;

public:
	typedef struct tagDesc : CComponent::DESC
	{
		std::string_view  sComTag;
	
	}DESC;


public:
	enum ANIMTYPE{
		ANIM, ACTION
	};

	enum SOUND_TYPE 
	{
		SOUND_2D,
		SOUND_3D,
		END
	};

	enum EVENTTYPE {
		COLLIDER,
		SOUND,
		EFFECT,
		END_EVENTTYPE
	};


	struct COLLIDER_EVENT_DESC 
	{
		std::string sColliderName;

		// Action 전체 기준 콜라이더 발생 위치
		_float fActionTrackPosition = 0.f;

		// 부착할 Bone 이름
		std::string sBoneName;

		// Bone 기준 Local Transform
		_float3 vLocalPosition = { 0.f, 0.f, 0.f };
		_float3 vLocalRotation = { 0.f, 0.f, 0.f };
		_float3 vLocalScale = { 1.f, 1.f, 1.f };
	};

	struct SOUND_EVENT_DESC {
		// ResourceManager에서 FMOD::Sound를 찾을 이름
		std::string sSoundName;

		// Action 전체 기준 사운드 재생 시점
		_float fActionTrackPosition = 0.f;

		SOUND_TYPE eSoundType = SOUND_TYPE::SOUND_3D;

		_float fVolume = 1.f;
		_float fPitch = 1.f;

		_bool bLoop = false;

		// 3D Sound의 발생 위치
		_bool bFollowOwner = true;

		// 특정 Bone에서 재생해야 할 경우
		std::string sBoneName;

		// Owner 또는 Bone 기준 로컬 위치
		_float3 vLocalPosition = { 0.f, 0.f, 0.f };
	};

	struct ANIMSTRUCT : public ISerializable
	{
		int32_t   iAnimIndex = -1;
		// 총 재생 시간
		_float   fDuration = 0.f;
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

		void SetTrackPostion(_float _trackPos) {
			fTrackPosition = _trackPos;
		}

		void Serialize(ISerializer& serializer) const override
		{
			serializer.Write("AnimIndex", iAnimIndex);
			serializer.Write("Duration", fDuration);
			serializer.Write("Speed", fSpeed);
			serializer.Write("Loop", bLoop);
		}

		void Deserialize(IDeserializer& deserializer) override
		{
			deserializer.Read("AnimIndex", iAnimIndex);
			deserializer.Read("Duration", fDuration);
			deserializer.Read("Speed", fSpeed);
			deserializer.Read("Loop", bLoop);
			fTrackPosition = 0.f;
			bFinished = false;
			KeyFrameIndices.clear();
		}

		_bool IsValid() const
		{
			return iAnimIndex >= 0;
		}




		//void Serialize(ISerializer& serializer) const
		//{
		//	WRITE_ALL(serializer, iAnimIndex, fTrackPosition, fSpeed, bLoop, bFinished, KeyFrameIndices);
		//	//serializer.Write("iAnimIndex", iAnimIndex);
		//	//serializer.Write("fTrackPosition", fTrackPosition);
		//	//serializer.Write("fSpeed", fSpeed);
		//	//serializer.Write("bLoop", bLoop);
		//	//serializer.Write("bFinished", bFinished);
		//	//serializer.Write("KeyFrameIndices", KeyFrameIndices);
		//}
		//void Deserialize(IDeserializer& deserializer)
		//{
		//	READ_ALL(deserializer, iAnimIndex, fTrackPosition, fSpeed, bLoop, bFinished, KeyFrameIndices);
		//	//deserializer.Read("iAnimIndex", iAnimIndex);
		//	//deserializer.Read("fTrackPosition", fTrackPosition);
		//	//deserializer.Read("fSpeed", fSpeed);
		//	//deserializer.Read("bLoop", bLoop);
		//	//deserializer.Read("bFinished", bFinished);
		//	//deserializer.Read("KeyFrameIndices", KeyFrameIndices);
		//}
	};
	 
	struct ACTIONSTRUCT : public ISerializable {
		std::string ActionName;
		
		// Animation 이벤트
		std::vector<ANIMSTRUCT> Anims;
		// Animation 이벤트 시작 분기
		std::vector<_float>     StartTime;
		// 최종 시간
		_float					LastTime;
		// Collider 이벤트
		std::vector<COLLIDER_EVENT_DESC> Colliders;
		// Sound 이벤트
		std::vector<SOUND_EVENT_DESC> Sounds;
		


		void Serialize(ISerializer& serializer) const
		{
			serializer.Write("ActionName", ActionName);
			serializer.Write("Anims", Anims);
			serializer.Write("StartTime", StartTime);
			serializer.Write("fLastTime", LastTime);
		
			serializer.Write("iAnimSize", Anims.size());
			
			// Animation 저장
			for (int32_t i = 0; i < (int32_t)Anims.size(); ++i) {
				serializer.Write("iAnimIndex", Anims[i].iAnimIndex);
				serializer.Write("fStartTime", StartTime[i]);
			}

			serializer.Write("fLastTime", LastTime);

			// Collider 저장

			serializer.Write("iColliderSize", Colliders.size());
			for (int32_t i = 0; i < (int32_t)Colliders.size(); ++i) {
			
				serializer.Write("fActionTrackPosition", Colliders[i].fActionTrackPosition);
				serializer.Write("sBoneName", Colliders[i].sBoneName);
				serializer.Write("sColliderName", Colliders[i].sColliderName);
				serializer.Write("vLocalPosition", Colliders[i].vLocalPosition);
				serializer.Write("vLocalRotation", Colliders[i].vLocalRotation);
				serializer.Write("vLocalScale", Colliders[i].vLocalScale);
			}

			// Sound저장 저장

			serializer.Write("iSoundSize", Sounds.size());
			for (int32_t i = 0; i < (int32_t)Sounds.size(); ++i) {

				serializer.Write("bFollowOwner", Sounds[i].bFollowOwner);
				serializer.Write("bLoop", Sounds[i].bLoop);
				serializer.Write("eSoundType", Sounds[i].eSoundType);
				serializer.Write("fActionTrackPosition", Sounds[i].fActionTrackPosition);
				serializer.Write("fPitch", Sounds[i].fPitch);
				serializer.Write("fVolume", Sounds[i].fVolume);
				serializer.Write("sBoneName", Sounds[i].sBoneName);
				serializer.Write("sSoundName", Sounds[i].sSoundName);
				serializer.Write("vLocalPosition", Sounds[i].vLocalPosition);
			}
		}
		void Deserialize(IDeserializer& deserializer)
		{
			deserializer.Read("ActionName", ActionName);
			deserializer.Read("Anims", Anims);
			deserializer.Read("StartTime", StartTime);
			deserializer.Read("fLastTime", LastTime);
			if (StartTime.size() < Anims.size())
				StartTime.resize(Anims.size(), 0.f);
		

			int32_t animsize{};
			deserializer.Read("iAnimSize", animsize);
			// Animation 저장
			for (int32_t i = 0; i < animsize; ++i) {
				deserializer.Read("iAnimIndex", Anims[i].iAnimIndex);
				deserializer.Read("fStartTime", StartTime[i]);
			}

			deserializer.Read("fLastTime", LastTime);

			// Collider 저장
			
			int32_t Collidersize{};
			deserializer.Read("iColliderSize", Collidersize);
			Colliders.clear();
			Colliders.resize(Collidersize);

			for (int32_t i = 0; i < Collidersize; ++i) {

				deserializer.Read("fActionTrackPosition", Colliders[i].fActionTrackPosition);
				deserializer.Read("sBoneName", Colliders[i].sBoneName);
				deserializer.Read("sColliderName", Colliders[i].sColliderName);
				deserializer.Read("vLocalPosition", Colliders[i].vLocalPosition);
				deserializer.Read("vLocalRotation", Colliders[i].vLocalRotation);
				deserializer.Read("vLocalScale", Colliders[i].vLocalScale);
			}

			// Sound저장 저장
			int32_t SoundSize;
			deserializer.Read("iSoundSize", SoundSize);
			Sounds.clear();

			Sounds.resize(SoundSize);
			for (int32_t i = 0; i < SoundSize; ++i) {

				deserializer.Read("bFollowOwner", Sounds[i].bFollowOwner);
				deserializer.Read("bLoop", Sounds[i].bLoop);
				deserializer.Read("eSoundType", Sounds[i].eSoundType);
				deserializer.Read("fActionTrackPosition", Sounds[i].fActionTrackPosition);
				deserializer.Read("fPitch", Sounds[i].fPitch);
				deserializer.Read("fVolume", Sounds[i].fVolume);
				deserializer.Read("sBoneName", Sounds[i].sBoneName);
				deserializer.Read("sSoundName", Sounds[i].sSoundName);
				deserializer.Read("vLocalPosition", Sounds[i].vLocalPosition);
			}
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
	enum class EVALUATION_MODE : uint32_t
	{
		CPU,
		GPU,
		CPU_GPU,
	};

	HRESULT Update(_float fTimeDelta);


	// AnimUpdate
	HRESULT	Update_Anim(_float fTimeDelta);
	void	Update_AnimState(_float fTimeDelta, ANIMSTRUCT& AnimState);
	void	Update_ActionState(_float fTimeDelta, ANIMSTRUCT& AnimState);
	void	Play_Anim(int32_t iAnimIndex, _bool bLoop=false, _float fBlendDuration = 0.1f);
	void	Play_Action(int32_t iActionIndex, _float fBlendDuration);
	void	Play_UpperAnim(int32_t iAnimIndex, _bool bLoop = false, _float fFadeDuration = 0.1f);
	void	Stop_UpperAnim(_float fFadeDuration = 0.1f);
	_bool	Set_UpperBodyRootBone(const _char* pBoneName, uint32_t iBlendDepth = 3);
	_bool	IsUpperLayerActive() const { return m_UpperAnimState.IsValid() && m_fUpperLayerWeight > 0.f; }
	void	Build_BoneMatrices_CPU(_float fTimeDelta);
	_bool Sample_CombinedBoneMatrices(int32_t iAnimIndex, _float fTrackPosition, const std::vector<uint32_t>& boneChain, _float4x4& outMatrix) const;

	void	Sample_Channel_CPU( CResModelChanel* pChannel, _float fTrackPosition, uint32_t& iCurrentKeyFrameIndex, std::vector<_float4x4>& OutLocalBoneMatrices);
	_matrix Evaluate_ChannelMatrix_CPU(CResModelChanel* pChannel, _float fTrackPosition) const;
	_matrix Evaluate_RootCombinedMatrix_CPU(const CResModelAnim* pAnim, _float fTrackPosition) const;
	void Prepare_RootMotionCache(const CResModelAnim* pAnim, _float fTrackPosition);
	void Apply_RootMotionFromCombined(_fmatrix currentRootCombined);
	void Update_RootMotion_GPU(const CResModelAnim* pAnim, _float fPreviousTrackPosition);
	void Invalidate_RootMotionCache();
	_vector RemoveYRotation(_vector qRotation) const;
	void	Blend_Anim(_float fTimeDelta);
	void	Update_UpperLayer(_float fTimeDelta);
	void	Build_UpperLocalPose();
	void	Compose_FinalLocalPose();


	// ActionUpdate
	HRESULT Update_Action(_float fTimeDelta);
	HRESULT Update_Anim_CPU_GPU(_float fTimeDelta);
	HRESULT Update_Action_CPU_GPU(_float fTimeDelta);

	HRESULT Update_Anim_GPU(_float fTimeDelta);

	HRESULT Update_Action_GPU(_float fTimeDelta);


	void	SetTrackPosition(
		_float fTrackPosition,
		_bool bPreserveBlend = false);


public:
	_float3 GetRootMotionDelta() { return m_vRootMotionDelta; }
	_float4 GetRootMotionRotationDelta() const { return m_qRootMotionRotationDelta; }
	ANIMSTRUCT& GetCurAnimState() { return  m_CurAnimState; }

public:
	std::vector<ACTIONSTRUCT>& GetActions()
	{
		return m_Actions;
	}

	const std::vector<ACTIONSTRUCT>& GetActions() const
	{
		return m_Actions;
	}

	int32_t GetCurrentActionIndex() const
	{
		return m_curActions;
	}

	int32_t GetCurrentActionAnimIndex() const
	{
		return m_curActionsAnim;
	}

	_float GetActionTime() const
	{
		return m_ActionTime;
	}

	uint32_t GetRootBoneIndex() const {
		return m_iRootBoneIndex;
	}

	_bool IsBlending() const { return m_bBlending && m_PrevAnimState.IsValid(); }
	const ANIMSTRUCT& GetPrevAnimState() const { return m_PrevAnimState; }
	_float GetBlendWeight() const
	{
		return m_fBlendDuration > 0.f ? std::clamp(m_fBlendTime / m_fBlendDuration, 0.f, 1.f) : 1.f;
	}
	void Advance_GPUBlend(_float fTimeDelta);
private:
	EVALUATION_MODE m_eEvaluationMode{ EVALUATION_MODE::GPU };

	CComModelInstance* m_pModelInstance;


private:
	_string			m_Comtag;
	ANIMTYPE		m_iPlayAnimationType{ ANIMTYPE::ANIM };
	int32_t			m_iPlayAnimationNum{ 0 };
	int32_t			m_iPlayAnimIndex{ 0 };


private:
	// 애니메이션 정보 (블렌딩 고려)
	ANIMSTRUCT m_CurAnimState;
	ANIMSTRUCT m_PrevAnimState;
	std::vector<ACTIONSTRUCT> m_Actions;
	// ActionAnim Num
	int32_t					 m_curActionsAnim;
	int32_t					 m_curActions;
	_float				     m_ActionTime{};
	// 애니메이션 제어를 위한 멤버 변수들
	_bool		    m_bPlay{ true };

	_bool			m_bBlending = true;
	_float          m_fRatio{ 0.f };
	_float			m_fBlendTime = 0.f;
	_float			m_fBlendDuration;

	std::vector<_float4x4>				m_LocalBoneMatrices;
	std::vector<_float4x4>				m_BlendStartLocalMatrices;
	std::vector<_float4x4>				m_UpperLocalBoneMatrices;
	std::vector<_float4x4>				m_FinalLocalBoneMatrices;
	std::vector<_float>					m_UpperBodyMask;

	ANIMSTRUCT		m_UpperAnimState;
	_float			m_fUpperLayerWeight{ 0.f };
	_float			m_fUpperFadeStartWeight{ 0.f };
	_float			m_fUpperFadeTargetWeight{ 0.f };
	_float			m_fUpperFadeTime{ 0.f };
	_float			m_fUpperFadeDuration{ 0.f };


private:
	_bool   m_bRootMotion{ true };
	int32_t m_iRootBoneIndex{ -1 };
	// 애니메이션 Local 기준 RootMotion
	_float3 m_vRootMotionDelta{ 0.f, 0.f, 0.f };
	_float4 m_qRootMotionRotationDelta{ 0.f, 0.f, 0.f, 1.f };
	_float4x4 m_RawRootLocalMatrix{};
	_float4x4 m_PreviousRootCombinedMatrix{};
	_bool m_bRawRootLocalValid{ false };
	_bool m_bPreviousRootCombinedValid{ false };
	int32_t m_iRootMotionCacheAnimIndex{ -1 };


public:
	static UPtr<CComAnimator> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

	_bool GetFinish() const { return m_CurAnimState.bFinished; }

	_bool GetPlay() const { return m_bPlay; }
	void  SetPlay(_bool bPlay) { m_bPlay = bPlay;}

	_bool GetLoop() const { return m_CurAnimState.bLoop; }
	void  SetLoop(_bool _bLoop) { m_CurAnimState.bLoop = _bLoop; }
	_float GetPlayAnimRatio() const { return m_fRatio; }


	
public:
	uint32_t GetAnimationTYPE() const { return ETOUI(m_iPlayAnimationType); }
	void SetAnimationTYPE(ANIMTYPE eType) { m_iPlayAnimationType = eType; }
	EVALUATION_MODE GetEvaluationMode() const { return m_eEvaluationMode; }
	void SetEvaluationMode(EVALUATION_MODE mode) { m_eEvaluationMode = mode; }

	uint32_t GetPlayAnimIndex() const { return m_CurAnimState.iAnimIndex; }
	void SetPlayAnimIndex(uint32_t iIndex) { m_iPlayAnimIndex = iIndex; }



};

NS_END
