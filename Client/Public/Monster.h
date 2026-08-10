#pragma once
#include "AnimationObject.h"
#include "Client_Defines.h"
NS_BEGIN(Engine)
class CComConstantBuffer;
class CResTexture2D;
class CResVertexShader;
class CResPixelShader;
class CResSamplerState;
class CResModel;
class CComModelInstance;
class CComAnimator;
class CComBeHavior;
class CComCollider;
class CComPxCharacterController;
class CComCharacterMoveIntent;
class CComCharacterMotor;
class CComPxRigidBody;
class CComPxSphereCollider;
class CComSound;
class CBTBlackBoard;
NS_END


NS_BEGIN(Client)
typedef struct HitTable
{
	_bool operator == (const HitTable& rhs) const
	{
		return (eAttType == rhs.eAttType) && (eHitType == rhs.eHitType);
	}

	ATTMON						eAttType{ ATTMON::END };
	PLAYER_SKILL_TYPE			eHitType{ PLAYER_SKILL_TYPE::DEFAULT };
	int32_t						iAnimIndex{ -1 };
	_float						fBlend{ 0.1f };

}HITTABLE;

typedef struct monsound
{
	SOUND_3D_DESC	 str3DSound{};
	SOUND_PLAY_DESC  SoundPlay{};
	_string			 SoundKey{};
	_float			 fCurRatioTime{};
	_float			 fPlayRatio{};

	_bool			 bOnlyOne{ false };
	_bool			 bPlayed{};
	SOUND_ID iSoundID{ INVALID_SOUND_ID };
}MONSOUND;

typedef struct MonsterHitInfo
{
	PLAYER_SKILL_TYPE eHitType{ PLAYER_SKILL_TYPE::DEFAULT};
	ATTMON    eAttType{ ATTMON::END };
	int32_t iPriority{ 0 };
}MON_HIT_INFO;
class CMonster : public CAnimationObject
{
public:
	DECLARE_DERIVED_TYPE(CMonster, CAnimationObject)
public:
	typedef struct tagGoblnedesc : CAnimationObject::GAMEOBJECT_DESC
	{
		_string SocketName{}, LevelTag{}, ReSourceTag{}, BeHaviorTag{};
		_bool	bDonMove{ false };
		_float3 vPos{}, vScale{ 1.f,1.f,1.f }, vRot{1.f,1.f,1.f};
		_float fAngle{};
		// 모델 로컬 크기이며 생성 시 vScale을 적용해 CCT 월드 크기로 변환한다.
		_float fCCTHeight{ 2.1f };
		_float fCCTRadius{ 0.45f };
		_float fCCTStepOffset{ 0.1f };
		_float3 vCCTCenterOffset{ 0.f, 1.5f, 0.f };

		_float3 vWeaponScale{ 1.f,1.f,1.f };
		_string resBeHaviorMajor{}, resBeHaviorMinor{};
		_string WeaponResourceName{};
		_string WeaponProtoName{};
		MONSTER_TYPE				MonType{MONSTER_TYPE::BOSS};
		CHandle						TargetHandle{};
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::ENEMY_BODY),
			.iSimulationMask = PX_ALL_LAYERS,
			// [LSY] 캐릭터 CCT끼리는 충돌하되 전투용 HurtBox는 이동 Query에서 제외한다.
			.iQueryMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
				ETOUI(COLLISION_LAYER::MOVING_PLATFORM) |
				ETOUI(COLLISION_LAYER::PLAYER_BODY) |
				ETOUI(COLLISION_LAYER::ENEMY_BODY)
		};


	}MONSTER_DESC;
protected:
	CMonster();
	~CMonster() override;

public:
	void UpdateGUI();
	HRESULT InitializePrototype(void* pArg);
	HRESULT Initialize(void* pArg) override;

	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch);
	HRESULT Update_InstanceBuffer(ID3D11DeviceContext* pContext, const std::vector<GPU_ANIM_INSTANCE_DATA>& Instances);
	HRESULT Bind_InstanceBuffer(ID3D11DeviceContext* pContext);
		
	/*----------- 광윤 추가 -----------*/
	HRESULT Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	bool	GetShadowBounds(BoundingBox& OutBounds) const override;
	/*---------------------------------*/
public:
	void OnTriggerEnter(CGameObject* pObj,const PX_ON_TRIGGER_DATA& info) override;
public:
	virtual _bool				Check_Table(PLAYER_SKILL_TYPE eType) { return true; };

	void						Set_Partes(PARTES eType, CHandle Handle) { m_Partes[ETOUI(eType)] = Handle; };
	const int32_t				Get_CurrentHp() const { return m_iHp; }
	const int32_t				Get_MaxHp()		const { return m_iMaxHp; }
	virtual const _float		Get_Damage() { return 0.f; }
	void						Set_Emissive(_float fEmissive) { m_fPreEmissive = m_fIntensive = fEmissive; }
	virtual _bool				Activate_PendingHit();
	const MON_HIT_INFO			Get_ActiveHitInfo()const { return m_ActiveMonTable; }
	const MON_HIT_INFO			Get_PendingHitInfo() const { return m_PendingMonTable; }
	_bool						Is_PendingHit() { return m_bPending; }
	_bool						Is_ActiveHit() { return m_bActiveHit; }
	void						ReActiveTable();
	_bool						Is_Grounded();
	_bool						Monster_Type(MONSTER_TYPE eType) { if (m_eMonType == eType)return true;  return false; }
	uint32_t					GetHitCnt() { return m_iHitCnt; }
	uint32_t					GetNormalCnt() {return m_iNormalHitCnt;}
	CGameObject*				Get_Target() { return CGameInstance::Get().GetGameObjectByHandle(m_TargetHandle); }

	SOUND_ID 					Play_Sound(const MONSOUND& MonSound);
	virtual void				Skill_Finished();
	virtual _string				Get_SkillName(ATTMON SkillNode) { return ""; };
	virtual void				Set_AttTable(ATTMON eType, _float2 fSkillRatio) {};
	void						Get_SoundKey(_string& CursoundName);
	const _float4x4*			Get_CombineBoneMatrix(int32_t iBoneIndex);
	CComAnimator*				Get_Animator();
	CComCharacterMoveIntent*	Get_MoveIntent();
	CBTBlackBoard*				Get_BlackBoard();
	int32_t						Find_AnimIndex(const _string& AnimName);
protected:
	uint32_t					Find_SkillNum(ATTMON eType);
	 _bool						Check_Flag(uint32_t iFlag);
	virtual	void				Damaged(PLAYER_SKILL_TYPE eType);
	void						Update_HurtBox();
	virtual void				Flag_Check(_float fTimeDelta);
private:
	void						Cancle_Attack();
	void						StartEmissive() { if (m_bWork) return;  m_bEmissive = true; }
	void						EmissiveFadeOut(_float fTimeDelta);
// 민수 추가 ----------------------------------------------------------
public:
	const _float3& GetHurtBoxPosition() const {return m_vHurtBoxPosition;}


protected:
	_float3 m_vHurtBoxPosition{};
// 민수 추가 ----------------------------------------------------------
protected:
	CComModelInstance* m_pComModelInstance{};
	CComAnimator* m_pModelAnimator{};
	CComBeHavior* m_pBeHavior;
	CComPxCharacterController* m_pCharacterController{};
	CComCharacterMoveIntent* m_pMoveIntent{};
	CComCharacterMotor* m_pCharacterMotor{};
	CComPxRigidBody* m_pComRigidBody{};
	CComPxSphereCollider* m_pComSphereCol{};
	CComSound* m_pComSound{};
	CHandle m_Partes[ETOUI(PARTES::END)]{};

	
	// Anim
	SPtr<CResPixelShader> m_pResPixelShader{};
	SPtr<CResVertexShader> m_pResVertexCPUSkinningInstancedShader{};
	SPtr<CResCBuffer> m_pResSkinMeshCBuffer{};



	CComConstantBuffer* m_pComCBufferPerObject{};
	_float3 m_fEMissiveColor{};
	_float ff{};

	_float2								m_fSkillRatio{ };
	uint32_t							m_iCurrentInstanceCount{}, m_iHitCnt{}, m_iNormalHitCnt{}, m_iCurEffectID{}, m_iPreSkill{}, m_iCurSkill{};
	_float								m_fIntensive{}, m_fPreEmissive{}, m_fAlpha{}, m_fTimeTick{}, m_fDamage{};
	int32_t								m_iHp{}, m_iMaxHp{}, m_iColliderBoneIndex{}, m_iEventBoneIndex{-1};
	_bool								m_bEmissive{ false }, m_bWork{ false },m_bSkillLoop{ false }, m_bSkipAtt{false};
	_string								m_SocketName{}, m_CurEffectName{};
	ATTMON								m_eAttType{ ATTMON::END },m_eLastSkillTable{ ATTMON::END };
	
	_bool								m_bPending{ false };
	MON_HIT_INFO						m_PendingMonTable{};

	_bool								m_bActiveHit{ false };
	MON_HIT_INFO						m_ActiveMonTable{};
	
	MONSTER_TYPE						m_eMonType{ MONSTER_TYPE::NORMAL };
	std::vector<E::SPAWN_COMMAND>		m_Effects[ETOUI(ATTMON::END)];
	CHandle								m_TargetHandle{};
	std::unordered_map<_string, std::vector<_string>> m_SoundTable;

	std::map<ATTMON, uint32_t>			m_MonSkillLists;
	//파티클 재설정용
	_bool								m_bDonMove{ false };
	std::map<ATTMON, _string>			m_ParticleData;
public:
	E::UPtr<E::CPrototype> Clone(void* pArg) PURE;
};

NS_END


