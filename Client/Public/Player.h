
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
class CResCBuffer;
class CComModelInstance;
class CComAnimator;
class CComSocket;


class CComPxRigidBody;
class CComPxBoxCollider;
class CComPxSphereCollider;
class CResPhysXBoxGeometry;
class CComPxCharacterController;
class CComCharacterMoveIntent;
class CComCharacterMotor;
class CComSound;
NS_END

NS_BEGIN(Client)
class CPlayer_StateMachine;
class CPlayerRagdollController;

class CPlayer final : public CAnimationObject
{
public:
	DECLARE_DERIVED_TYPE(CPlayer, CAnimationObject)

public:
	enum class PLAYER_COLLISIONS : uint32_t
	{
		CCT_CAPSULE = 0,
		PLAYER_SHAPE_HURTBOX,
		PLAYER_LEFT_FOOT,
		PLAYER_RIGHT_FOOT,
		END
	};

protected:
	void UpdateGUI() override;

public:
	struct DESC : public CGameObject::GAMEOBJECT_DESC
	{
		_float3 vInitialPosition{ 50.f, 50.f, 10.f };
		_float3 vInitialRotation{};
		// PhysX capsule height는 양 끝 반구를 제외한 원통 부분의 높이다.
		_float fCCTHeight{ 3.6f };
		_float fCCTRadius{ 0.6f };
		_float fCCTStepOffset{ 0.1f };
		_float3 vCCTCenterOffset{ 0.f, 0.9f, 0.f };
		PX_FILTER_DESC tFilter{
			.iLayer = ETOUI(COLLISION_LAYER::PLAYER_BODY),
			.iSimulationMask = PX_ALL_LAYERS,
			// [LSY] 적 CCT와는 충돌하되 전투용 HurtBox는 이동 Query에서 제외한다.
			.iQueryMask =
				ETOUI(COLLISION_LAYER::WORLD_STATIC) |
				ETOUI(COLLISION_LAYER::WORLD_DYNAMIC) |
				ETOUI(COLLISION_LAYER::MOVING_PLATFORM) |
				ETOUI(COLLISION_LAYER::ENEMY_BODY)
		};
		StringID LevelTag;
		CHandle  UIHandle;
	};

private:
	CPlayer();
	CPlayer(const CPlayer& Prototype);
	~CPlayer() override;


public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void FixedUpdate(_float fTimeDelta) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch) override;
	HRESULT Update_InstanceBuffer(ID3D11DeviceContext* pContext, const std::vector<GPU_ANIM_INSTANCE_DATA>& Instances);

	/*----------- 광윤 추가 -----------*/
	bool	GetShadowBounds(BoundingBox& OutBounds) const override;
	/*---------------------------------*/

	HRESULT Bind_InstanceBuffer(ID3D11DeviceContext* pContext);
public:
	HRESULT Hit_Player_HurtBox(CGameObject* pAttacker, const PX_ON_COLLISION_DATA& info);
	_bool OnQueryHit(CGameObject* pAttacker,const PX_OVERLAP_RESULT& tHit,int32_t iDamage,const _float3& vHitPosition);
	_bool OnQueryHit(int32_t iDamage,const _float3& vHitPosition);
	_bool OnQueryHit(int32_t iDamage);
	int32_t GetCurrentHp() const { return m_iHp; }
	int32_t GetMaxHp() const { return m_iMaxHp; }
	const _float3& GetLastHitPosition() const { return m_vLastHitPosition; }
private:
	void HandleDeath();
public:
	void Attack_Magic_Bullet();
public:
	void OnWake() override;
	void OnSleep() override;
	void OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override;
	void OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override;
	void OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override;
	void OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override;
	void PlayFootstepSound(PLAYER_COLLISIONS eFoot);

	CComCharacterMoveIntent* GetMoveIntent() const { return m_pComMoveIntent; }
	CComCharacterMotor* GetCharacterMotor() const { return m_pComCharacterMotor; }
	CComAnimator* GetAnimator() const { return m_pModelAnimator; }
	CComModelInstance* GetModelInstance() const { return m_pComModelInstance; }
	CHandle GetTargetHandle() const { return m_hAutoTarget; }
	CHandle GetUIControllerHandle() const { return m_UIHandle; }
	PLAYER_SKILL_TYPE GetPlayerCurSkill() const { return m_eSkill_Type; }

	void SetPlayerCurSKill(PLAYER_SKILL_TYPE _Skill_Type) { m_eSkill_Type = _Skill_Type; }
	void SetMovementLocked(_bool bLocked) { m_bMovementLocked = bLocked; }
	void SetRootMotionRotationActive(_bool bActive) { m_bRootMotionRotationActive = bActive; }
	void SetRootMotionTranslationActive(_bool bActive) { m_bRootMotionTranslationActive = bActive; }
	void ApplyAttackForwardMovement(_float fSpeed, _float fTimeDelta);
	void ApplyDirectionalMovement(const _float3& vDirection,_float fSpeed,_float fTimeDelta);
	void ApplyGroundFollow(_float fFixedTimeDelta);
	void PrepareLocomotionResume();
	void InitializeSkillSlotUI();
	_bool TryUseSkillSlot(uint32_t iSlotNumber);
	_bool HasRawMoveInput() const { return m_bRawMoveInput; }
	_bool IsSprintRequested() const { return m_bSprintRequested; }
	const _float3& GetRawMoveDirection() const { return m_vRawMoveDirection; }
	_float GetCurrentMoveSpeed() const { return m_fCurrentMoveSpeed; }
	void SetCurrentMoveSpeed(_float fSpeed) { m_fCurrentMoveSpeed = std::max(0.f, fSpeed); }
	void SetFlyRequested(_bool bRequested);
	_bool IsFlyRequested() const { return m_bFlyRequested; }
	
	_bool GetRenderInfluence() { return m_bRenderInfluence; }
	void SetRenderInfluence(_bool _RenderInfluence) { m_bRenderInfluence = _RenderInfluence; }


	void SetBodyEffectID(uint32_t effectID) { m_iDashBodyEffectID = effectID; }
	void UpdateAttachedEffects();
	CHandle& GetWeaponHandle() { return m_Partes[ETOUI(PARTES::WEAPON)]; }


	_bool GetInvincible() const { return m_bInvincible; }
	void SetInvincible(_bool bInvincible) { m_bInvincible = bInvincible; }
private:
	CComModelInstance* m_pComModelInstance{};
	CComAnimator* m_pModelAnimator{};

	// Anim
	SPtr<CResPixelShader> m_pResPixelShader{};

	SPtr<CResVertexShader> m_pResVertexCPUSkinningInstancedShader{};

	SPtr<CResCBuffer> m_pResSkinMeshCBuffer{};
	CHandle m_Partes[ETOUI(PARTES::END)]{};

	CComConstantBuffer* m_pComCBufferPerObject{};
	CComSocket* m_pSocket;


	_float4 m_fAlbedoColor = { 1.f, 1.f, 1.f, 1.f };
	_float	m_fNormalIntensity = 1.f;
	_float	m_fRoughnessIntensity = 1.f;
	_float	m_fMetallicIntensity = 1.f;
	_float	m_fAmbientIntensity = 1.f;
	_float	m_fSpecularIntensity = 1.f;
	_float3 m_fEmissiveColor = { 1.f, 1.f, 1.f };
	_float	m_fEmissiveIntensity = 0.f;

	_float3 m_vInitialPosition{};

	uint32_t m_iDebugSelectedBone = 0;
	uint32_t m_iCurrentInstanceCount = 0;
	uint32_t m_iDashBodyEffectID = INVALID_EFFECT_INSTANCE_ID;

private:
	_bool	 m_bRenderInfluence{ false	 };
	_bool m_bFlyRequested{};

private:
	struct PROJECTILE_LIFETIME
	{
		CHandle hProjectile{};
		_float fRemainingTime{};
	};

	CComPxRigidBody* m_pComPxRigidBody{};
	CComPxBoxCollider* m_pComPxBoxCollider{};
	CComPxSphereCollider* m_pComPxLeftFootCollider{};
	CComPxSphereCollider* m_pComPxRightFootCollider{};
	int32_t m_iHurtBoxBoneIndex{ -1 };
	int32_t m_iLeftFootBoneIndex{ -1 };
	int32_t m_iRightFootBoneIndex{ -1 };
	_float m_fFootstepSoundCooldown{};
	CComCollider* m_pComCollider{};
	CComPxCharacterController* m_pComCharacterController{};
	CComCharacterMoveIntent* m_pComMoveIntent{};
	CComCharacterMotor* m_pComCharacterMotor{};
	CPlayer_StateMachine* m_pStateMachine{};
	_bool m_bMovementLocked{};
	_bool m_bRootMotionRotationActive{};
	_bool m_bRootMotionTranslationActive{};
	_bool m_bRawMoveInput{};
	_bool m_bSprintRequested{};
	_float3 m_vRawMoveDirection{};
	_float3 m_vLastMoveDirection{ 0.f, 0.f, 1.f };
	_float3 m_vSmoothedMoveDirection{ 0.f, 0.f, 1.f };
	_float m_fCurrentMoveSpeed{};
	_float m_fJogSpeed{ 7.5f };
	_float m_fSprintSpeed{ 15.f };
	_float m_fAcceleration{ 12.f };
	_float m_fDeceleration{ 18.f };
	_float m_fJogDirectionResponse{ 7.f };
	_float m_fSprintDirectionResponse{ 4.5f };
	int32_t m_iHp{ 500 };
	int32_t m_iMaxHp{ 500 };

	_bool m_bDeathEventPublished{};
	_float3 m_vLastHitPosition{};
	_float m_fGroundFollowProbeStartHeight{ 0.1f };
	_float m_fGroundFollowMaxStepDown{ 0.5f };
	_float m_fGroundFollowProbeRadius{ 0.2f };
	_bool  m_bInvincible{ false };
	std::vector<PROJECTILE_LIFETIME> m_Projectiles{};

	//[LSY] 테스트 로그니 지우셔도 됩니다.
#ifdef _DEBUG
	void UpdateStandingGameObjectDebugLog();
	std::optional<CHandle> m_hDebugStandingGameObject{};
#endif

private:
	CHandle m_hAutoTarget;
	CHandle m_hPrevAutoTarget;
	CHandle m_hMonsterHPUITarget{};
	StringID m_LevelTag;
private:
	CHandle m_UIHandle;
	_bool m_bSkillSlotUIInitialized{ false };

#pragma region RAGDOLL
	friend class CPlayerRagdollController;
public:
	_bool RequestRagdollActivation(
		const _float3& vLinearVelocity = {},
		const _float3& vAngularVelocityRadians = {});
	_bool ResetRagdoll();
	_bool IsRagdollActive() const;
	_bool TryGetRagdollFollowPosition(_float3& OutPosition) const;
private:
	HRESULT InitializeRagdoll();
	_bool IsRagdollTransitioning() const;
private:
	UPtr<CPlayerRagdollController> m_pRagdollController{};
#pragma endregion

private:
	PLAYER_SKILL_TYPE m_eSkill_Type;
private:
	static constexpr _float DASH_HOLD_TIME = 0.35f;

	_float m_fControlHoldTime{};
	_bool m_bDashTriggered{};


private:
	_float m_fCoolTime_Num1{ 0.f };
	_bool m_bCoolTime_Num1{ false};
	_float m_fCoolTime_Num2{ 0.f };
	_bool m_bCoolTime_Num2{ false };
	_float m_fCoolTime_Num3{ 0.f };
	_bool m_bCoolTime_Num3{ false };
	_float m_fCoolTime_Num4{ 0.f };
	_bool m_bCoolTime_Num4{ false };

private:
	void DelayFinish(_float fTimeDelta);
private:
	_float m_fDelayTime{ 0.f };
private:
	_bool  m_bUI = false;
	_bool  m_bDistanceUI = false;
	CHandle m_hUI;

private:
	CComSound* m_pComSound{};
public:
	CComSound* GetSound() const { return m_pComSound; }
public:
	static E::UPtr<CPlayer> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;


private:
	//성민 지울거임
	uint32_t testEffectID = 0;
	_float	m_fDistanceOffeset = 1.6f;
	_float3	m_vSpwanPos{};
};

NS_END
