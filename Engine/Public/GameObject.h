#pragma once
#include "Prototype.h"
#include "MyTreeNode.h"
#include "Component.h"
#include "ComTransform.h"
#include "ComCollider.h"
#include "Handle.h"
#include "IRenderable.h"
#include "IPhysicsListener.h"
#include "IPhysicsSync.h"

NS_BEGIN(Engine)

struct MODEL_INSTANCE_BATCH;
class CGameObjectManager;
class CGameObjectPoolManager;

class ENGINE_DLL CGameObject : public CPrototype,
								public IRenderable,
								public IPhysicsListener,
								public IPhysicsSync,
								public CMyTreeNode<CGameObject>
{
public:
	DECLARE_DERIVED_TYPE(CGameObject, CPrototype)
	// ENGINE_DLL 인애들은 반드시 명시적으로 복사 생성자, 복사 대입연산자 딜리트하거나 재정의해주어야함
	CGameObject& operator=(const CGameObject&) = delete;

public:
	typedef struct tagGameObjectDesc
	{
		CHandle __handle{};
		_string sObjectTag{};
	}GAMEOBJECT_DESC;

protected:
	explicit CGameObject();
	explicit CGameObject(const CGameObject& Prototype);
	~CGameObject();

public:
	virtual HRESULT Initialize(void* pArg);
	virtual void FixedUpdate(_float fTimeDelta);
	virtual void PriorityUpdate(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual void LateUpdate(_float fTimeDelta);
	virtual void UpdateGUI();

protected:
	// [LSY] GameObjectManager의 슬롯과 레이어에 등록이 모두 끝난 직후 한 번 호출된다.
	// Initialize 중에는 아직 자기 Handle 조회가 불가능하므로 Manager 등록이 필요한 후처리는 여기서 수행한다.
	virtual void OnRegisteredToManager() {}

public:
	HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
	virtual HRESULT Render_Instanced(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx, const MODEL_INSTANCE_BATCH& Batch) { return S_OK; }

	virtual void SetInstanceModelNum(uint32_t iInstanceNum) {}
	bool HasRenderPass(RENDERPASS ePass) const override { return (m_RenderPassFlags & static_cast<uint32_t>(ePass)) != 0; };

	virtual _bool GetShadowBounds(BoundingBox& OutBounds) const override { return false; }
protected:
	uint32_t m_RenderPassFlags = ETOUI(RENDERPASS::DEFAULT);

public:
	CComTransform& GetTransform()				{ return *m_pComTransform;	}
	const CComTransform& GetTransform() const	{ return *m_pComTransform;	}

	
	CComCollider& GetCollider()					{ return *m_pComCollider;	}
	const CComCollider& GetCollider() const		{ return *m_pComCollider;	}

protected:
	std::vector<std::pair<StringID, UPtr<CComponent>>> m_Components{};
	std::unordered_map<StringID, size_t> m_ComponentsLookup{};
	CComTransform*	m_pComTransform{};

	CComCollider*	m_pComCollider{};

private:
	UPtr<CPrototype> CloneComponentProtoType(const StringID& svGroupTag, const StringID& svPrototypetag, void* pArg) const;

public:
	template<typename T>
	HRESULT AddComponentFromProto(const StringID& svGroupTag, const StringID& svPrototypetag, const StringID& svComponentTag,  void* pArg = nullptr, T** outPtr = nullptr)
	{
		auto pCom = CloneComponentProtoType(svGroupTag, svPrototypetag, pArg);
		if (!pCom)
		{
			return E_FAIL;
		}

		T* pCache = AddComponent(svComponentTag, static_uptr_cast<T>(std::move(pCom)));

		if (outPtr)
		{
			*outPtr = pCache;
		}

		return S_OK;
	}



	template<typename T>
	T* AddComponent(const StringID& tagComponent, UPtr<T> pComponent)
	{
		auto iter = m_ComponentsLookup.find(tagComponent);
		if (iter != m_ComponentsLookup.end())
		{
			return nullptr;
		}

		//pComponent->SetGameObject(this);

		T* pCache = pComponent.get();

		auto size = m_Components.size();
		std::pair<StringID, UPtr<CComponent>> a{ tagComponent, std::move(pComponent) };
		m_Components.push_back(std::move(a));
		m_ComponentsLookup.emplace(tagComponent, size);


		return pCache;
	}

	template<typename T>
	T* GetComponent(const StringID& tagComponent)
	{
		auto iter = m_ComponentsLookup.find(tagComponent);
		if (iter == m_ComponentsLookup.end())
		{
			return nullptr;
		}
		
		if (m_Components[iter->second].second->IsA(T::StaticType))
		{
			return static_cast<T*>(m_Components[iter->second].second.get());
		}

		return nullptr;
	};



protected:
	HRESULT DelComponent(const StringID& tagComponent);

public:
	_string_view GetObjectTag() const { return m_sObjectTag; }
	void SetObjectTag(_string_view sObjectTag) { m_sObjectTag = sObjectTag; }
protected:
	_string m_sObjectTag{};


private:
	CHandle m_ObjectHandle{};
public:
	const CHandle& GetHandle() const { return m_ObjectHandle; }

protected:
	void Free() override;

public:
	void SetPendingDestroy(_bool b = true);
	void SetPendingDestroyCascade(_bool b = true);
	_bool GetPendingDestroy() const { return m_bPendingDestroy; }
private:
	_bool m_bPendingDestroy{ false };

public:
	// [LSY] GameObjectManager가 호출하는 Fixed/Priority/Update/LateUpdate 참여 여부만 제어한다.
	void SetManagedUpdateEnabled(_bool bEnabled);
	void SetManagedUpdateEnabledCascade(_bool bEnabled);
	_bool IsManagedUpdateEnabled() const { return m_bManagedUpdateEnabled; }
protected:
	virtual void OnManagedUpdateEnabled() {}
	virtual void OnManagedUpdateDisabled() {}
	virtual _bool OnAcquireFromPool(void* pArg) { return true; }
	virtual void OnReleaseToPool() {}
private:
	// Pool Manager를 거치지 않은 직접 호출은 Manager의 Handle 상태와 어긋나므로 차단한다.
	_bool AcquireFromPool(void* pArg = nullptr);
	void ReleaseToPool();
	_bool m_bManagedUpdateEnabled{ true };
	friend class CGameObjectManager;
	friend class CGameObjectPoolManager;

	// IPhysicsListener
public:
	void OnWake() override {}
	void OnSleep() override {}
	void OnCollisionEnter(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override {}
	void OnCollisionExit(CGameObject* pObj, const PX_ON_COLLISION_DATA& info) override {}
	void OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override {}
	void OnTriggerExit(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info) override {}
	void OnJointBreak(const PX_ON_JOINT_BREAK_DATA& tData) override {}

	// CCT Hit 알림은 move 중 즉시 호출되지 않고 물리 이벤트 Dispatch 시점에 전달된다.
	void OnCCTShapeHit(const PX_CCT_HIT_DATA& tHit) override {}
	void OnCCTControllerHit(const PX_CCT_HIT_DATA& tHit) override {}
	void OnCCTObstacleHit(const PX_CCT_OBSTACLE_HIT_DATA& tHit) override {}

	// CCT 이동 계산에 즉시 사용되므로 상태 변경 없이 정책값만 반환해야 한다.
	PX_CCT_BEHAVIOR GetCCTShapeBehavior(CGameObject* pGameObject) const override
	{
		return PX_CCT_BEHAVIOR::CAN_RIDE;
	}
	PX_CCT_BEHAVIOR GetCCTControllerBehavior(CGameObject* pGameObject) const override
	{
		// [LSY] CCT가 다른 CCT 위에 멈추지 않고 캡슐 표면을 따라 미끄러지게 한다.
		return PX_CCT_BEHAVIOR::SLIDE;
	}
	PX_CCT_BEHAVIOR GetCCTObstacleBehavior(const void* pUserData) const override
	{
		return PX_CCT_BEHAVIOR::CAN_RIDE;
	}

	// IPhysicsSync
public:
	void SyncActivePhysXData(const PX_SYNC_DATA& syncData) override;
	virtual void UpdatePhysicData();
private:
	PX_SYNC_DATA m_PhysXSyncData{};
	_bool m_bPhysXSynced{ false };
};

NS_END
