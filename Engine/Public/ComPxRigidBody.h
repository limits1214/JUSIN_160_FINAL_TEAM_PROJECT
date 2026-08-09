#pragma once
#include "Component.h"
NS_BEGIN(physx)
class PxRigidActor;
NS_END


NS_BEGIN(Engine)

class CComPxJoint;

class ENGINE_DLL CComPxRigidBody : public CComponent
{
public:
	enum class TYPE { STATIC, DYNAMIC, KINEMATIC };
	struct DESC : public CComponent::DESC
	{
		TYPE eType = TYPE::DYNAMIC;
		float   fMass = 1.0f;
		XMFLOAT3 vPosition = { 0.f, 0.f, 0.f };
		XMFLOAT4 vRotation = { 0.f, 0.f, 0.f, 1.f }; // Quaternion
	};
public:
	DECLARE_DERIVED_TYPE(CComPxRigidBody, CComponent)

public:
	void UpdateGUI() override;

public:
	physx::PxRigidActor* GetActor() const { return m_pActor; }
	TYPE GetRigidBodyType() const { return m_eType; }
	bool IsDynamic() const { return m_bIsDynamic; }
	float GetMass() const { return m_fMass; }

	_bool SetPosition(const _float3& vPosition);
	_float3 GetPosition() const;
	_bool SetRotation(const _float4& vQuaternion);
	_float4 GetRotation() const;
	_bool SetPose(const _float3& vPosition, const _float4& vQuaternion);
	_bool SetMass(_float fMass);
	_bool SetKinematic(_bool bKinematic);
	_bool IsKinematic() const;

	_float3 GetLinearVelocity() const;
	_bool SetLinearVelocity(const _float3& vVelocity);
	_float3 GetAngularVelocity() const;
	_bool SetAngularVelocity(const _float3& vVelocity);

	_bool AddForce(const _float3& vForce);
	_bool AddImpulse(const _float3& vImpulse);
	_bool AddTorque(const _float3& vTorque);
	// 위치만 변경하고 현재 Actor 회전은 유지한다.
	_bool SetKinematicTarget(const _float3& vPosition);
	_bool SetKinematicTarget(const _float3& vPosition, const _float4& vQuaternion);

	_bool SetGravityEnabled(_bool bEnabled);
	_bool IsGravityEnabled() const;
	_bool SetLinearDamping(_float fDamping);
	_bool SetAngularDamping(_float fDamping);
	_bool SetMaxDepenetrationVelocity(_float fVelocity);
	_bool WakeUp();
	_bool PutToSleep();
	_bool IsSleeping() const;
	// Pool 반환이나 상태 전환 전에 이 RigidBody와 연결된 모든 Joint를 안전하게 해제한다.
	void ReleaseConnectedJoints();

private:
	explicit CComPxRigidBody();
	~CComPxRigidBody() override;

private:
	HRESULT Initialize(void* pArg) override;

private:
	physx::PxRigidActor* m_pActor{};
	bool          m_bIsDynamic = true;
	float   m_fMass{};
	TYPE m_eType{};
	std::unordered_set<CComPxJoint*> m_Joints{};

private:
	void RegisterJoint(CComPxJoint* pJoint);
	void UnregisterJoint(CComPxJoint* pJoint);

public:
	static UPtr<CComPxRigidBody> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

private:
	void Free() override;

	friend class CComPxJoint;
};

NS_END
