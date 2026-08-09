#include "pch.h"
#include "PhysxManagerListener.h"
#include "PhysXManager.h"

NS_USING(Engine)


CPhysxManagerListener::CPhysxManagerListener()
{

}
CPhysxManagerListener::~CPhysxManagerListener()
{

}

HRESULT CPhysxManagerListener::Initialize()
{
	m_PendingEvents.reserve(128);
    return S_OK;
}

void CPhysxManagerListener::PushEvent(EVENT_TYPE eType, const CHandle& hObjectA, const CHandle& hObjectB)
{
	std::lock_guard lock{ m_PendingEventMutex };
	m_PendingEvents.push_back({ eType, hObjectA, hObjectB });
}

void CPhysxManagerListener::PushCollisionEvent(
	EVENT_TYPE eType, const CHandle& hObjectA, const CHandle& hObjectB, const PX_ON_COLLISION_DATA& tData)
{
	PENDING_EVENT tEvent{};
	tEvent.eType = eType;
	tEvent.hObjectA = hObjectA;
	tEvent.hObjectB = hObjectB;
	tEvent.tCollision = tData;

	std::lock_guard lock{ m_PendingEventMutex };
	m_PendingEvents.push_back(tEvent);
}

void CPhysxManagerListener::PushTriggerEvent(
	EVENT_TYPE eType, const CHandle& hObjectA, const CHandle& hObjectB, const PX_ON_TRIGGER_DATA& tData)
{
	PENDING_EVENT tEvent{};
	tEvent.eType = eType;
	tEvent.hObjectA = hObjectA;
	tEvent.hObjectB = hObjectB;
	tEvent.tTrigger = tData;

	std::lock_guard lock{ m_PendingEventMutex };
	m_PendingEvents.push_back(tEvent);
}

void CPhysxManagerListener::PushJointBreakEvent(
	const PX_ON_JOINT_BREAK_DATA& tData)
{
	PENDING_EVENT tEvent{};
	tEvent.eType = EVENT_TYPE::JOINT_BREAK;
	tEvent.hObjectA = tData.hJointOwner;
	tEvent.tJointBreak = tData;

	std::lock_guard lock{ m_PendingEventMutex };
	m_PendingEvents.push_back(tEvent);
}

void CPhysxManagerListener::QueueCCTShapeHit(
	const CHandle& hOwner, const CHandle& hOther, const PX_CCT_HIT_DATA& tHit)
{
	PENDING_EVENT tEvent{};
	tEvent.eType = EVENT_TYPE::CCT_SHAPE_HIT;
	tEvent.hObjectA = hOwner;
	tEvent.hObjectB = hOther;
	tEvent.tCCTHit = tHit;
	tEvent.tCCTHit.pGameObject = nullptr;

	std::lock_guard lock{ m_PendingEventMutex };
	m_PendingEvents.push_back(tEvent);
}

void CPhysxManagerListener::QueueCCTControllerHit(
	const CHandle& hOwner, const CHandle& hOther, const PX_CCT_HIT_DATA& tHit)
{
	PENDING_EVENT tEvent{};
	tEvent.eType = EVENT_TYPE::CCT_CONTROLLER_HIT;
	tEvent.hObjectA = hOwner;
	tEvent.hObjectB = hOther;
	tEvent.tCCTHit = tHit;
	tEvent.tCCTHit.pGameObject = nullptr;

	std::lock_guard lock{ m_PendingEventMutex };
	m_PendingEvents.push_back(tEvent);
}

void CPhysxManagerListener::QueueCCTObstacleHit(
	const CHandle& hOwner, const PX_CCT_OBSTACLE_HIT_DATA& tHit)
{
	PENDING_EVENT tEvent{};
	tEvent.eType = EVENT_TYPE::CCT_OBSTACLE_HIT;
	tEvent.hObjectA = hOwner;
	tEvent.tCCTObstacleHit = tHit;

	std::lock_guard lock{ m_PendingEventMutex };
	m_PendingEvents.push_back(tEvent);
}

void CPhysxManagerListener::DispatchPendingEvents()
{
	std::vector<PENDING_EVENT> pendingEvents{};
	{
		std::lock_guard lock{ m_PendingEventMutex };
		pendingEvents.swap(m_PendingEvents);
	}

	for (const PENDING_EVENT& tEvent : pendingEvents)
	{
		// [LSY] 풀로 반환된 객체에는 큐에 남아 있던 지연 이벤트를 전달하지 않는다.
		CGameObject* pObjectA = CGameInstance::Get().GetGameObjectByHandle(tEvent.hObjectA);
		if (!pObjectA || pObjectA->GetPendingDestroy() ||
			!pObjectA->IsManagedUpdateEnabled())
			continue;

		if (tEvent.eType == EVENT_TYPE::WAKE)
		{
			pObjectA->OnWake();
			continue;
		}

		if (tEvent.eType == EVENT_TYPE::SLEEP)
		{
			pObjectA->OnSleep();
			continue;
		}

		if (tEvent.eType == EVENT_TYPE::JOINT_BREAK)
		{
			pObjectA->OnJointBreak(tEvent.tJointBreak);
			continue;
		}

		if (tEvent.eType == EVENT_TYPE::CCT_SHAPE_HIT ||
			tEvent.eType == EVENT_TYPE::CCT_CONTROLLER_HIT ||
			tEvent.eType == EVENT_TYPE::CCT_OBSTACLE_HIT)
		{
			if (tEvent.eType == EVENT_TYPE::CCT_OBSTACLE_HIT)
			{
				pObjectA->OnCCTObstacleHit(tEvent.tCCTObstacleHit);
				continue;
			}

			PX_CCT_HIT_DATA tHit = tEvent.tCCTHit;
			tHit.pGameObject = CGameInstance::Get().GetGameObjectByHandle(tEvent.hObjectB);
			if (tHit.pGameObject &&
				(tHit.pGameObject->GetPendingDestroy() ||
				 !tHit.pGameObject->IsManagedUpdateEnabled()))
			{
				tHit.pGameObject = nullptr;
			}

			if (tEvent.eType == EVENT_TYPE::CCT_SHAPE_HIT)
				pObjectA->OnCCTShapeHit(tHit);
			else
				pObjectA->OnCCTControllerHit(tHit);
			continue;
		}

		CGameObject* pObjectB = CGameInstance::Get().GetGameObjectByHandle(tEvent.hObjectB);
		if (!pObjectB || pObjectB->GetPendingDestroy() ||
			!pObjectB->IsManagedUpdateEnabled())
			continue;

		switch (tEvent.eType)
		{
		case EVENT_TYPE::COLLISION_ENTER:
		{
			pObjectA->OnCollisionEnter(pObjectB, tEvent.tCollision);
			PX_ON_COLLISION_DATA tOtherData = tEvent.tCollision;
			std::swap(tOtherData.eSelfShapeType, tOtherData.eOtherShapeType);
			std::swap(tOtherData.iSelfShapeSubIndex, tOtherData.iOtherShapeSubIndex);
			for (uint32_t i = 0; i < tOtherData.iContactCount; ++i)
			{
				tOtherData.Contacts[i].vWorldNormal.x *= -1.f;
				tOtherData.Contacts[i].vWorldNormal.y *= -1.f;
				tOtherData.Contacts[i].vWorldNormal.z *= -1.f;
				tOtherData.Contacts[i].vImpulse.x *= -1.f;
				tOtherData.Contacts[i].vImpulse.y *= -1.f;
				tOtherData.Contacts[i].vImpulse.z *= -1.f;
			}
			pObjectB->OnCollisionEnter(pObjectA, tOtherData);
			break;
		}
		case EVENT_TYPE::COLLISION_EXIT:
		{
			pObjectA->OnCollisionExit(pObjectB, tEvent.tCollision);
			PX_ON_COLLISION_DATA tOtherData = tEvent.tCollision;
			std::swap(tOtherData.eSelfShapeType, tOtherData.eOtherShapeType);
			std::swap(tOtherData.iSelfShapeSubIndex, tOtherData.iOtherShapeSubIndex);
			pObjectB->OnCollisionExit(pObjectA, tOtherData);
			break;
		}
		case EVENT_TYPE::TRIGGER_ENTER:
		{
			pObjectA->OnTriggerEnter(pObjectB, tEvent.tTrigger);
			PX_ON_TRIGGER_DATA tOtherData = tEvent.tTrigger;
			tOtherData.bSelfIsTrigger = !tOtherData.bSelfIsTrigger;
			std::swap(tOtherData.eSelfShapeType, tOtherData.eOtherShapeType);
			std::swap(tOtherData.iSelfShapeSubIndex, tOtherData.iOtherShapeSubIndex);
			pObjectB->OnTriggerEnter(pObjectA, tOtherData);
			break;
		}
		case EVENT_TYPE::TRIGGER_EXIT:
		{
			pObjectA->OnTriggerExit(pObjectB, tEvent.tTrigger);
			PX_ON_TRIGGER_DATA tOtherData = tEvent.tTrigger;
			tOtherData.bSelfIsTrigger = !tOtherData.bSelfIsTrigger;
			std::swap(tOtherData.eSelfShapeType, tOtherData.eOtherShapeType);
			std::swap(tOtherData.iSelfShapeSubIndex, tOtherData.iOtherShapeSubIndex);
			pObjectB->OnTriggerExit(pObjectA, tOtherData);
			break;
		}
		default:
			break;
		}
	}

	pendingEvents.clear();
	{
		std::lock_guard lock{ m_PendingEventMutex };
		if (m_PendingEvents.empty())
			m_PendingEvents.swap(pendingEvents);
	}
}

void CPhysxManagerListener::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
{
	if (!constraints)
		return;

	for (physx::PxU32 i = 0; i < count; ++i)
	{
		const physx::PxConstraintInfo& tConstraintInfo =
			constraints[i];
		if (tConstraintInfo.type !=
				physx::PxConstraintExtIDs::eJOINT ||
			!tConstraintInfo.externalReference)
		{
			continue;
		}

		auto* pPxJoint = static_cast<physx::PxJoint*>(
			tConstraintInfo.externalReference);
		if (!pPxJoint->userData)
			continue;

		const auto* pJointData =
			static_cast<const PX_JOINT_USER_DATA*>(
			pPxJoint->userData);
		PX_ON_JOINT_BREAK_DATA tBreakData{};
		tBreakData.hJointOwner = pJointData->hJointOwner;
		tBreakData.hActorA = pJointData->hActorA;
		tBreakData.hActorB = pJointData->hActorB;
		tBreakData.iJointSubIndex =
			pJointData->iJointSubIndex;
		PushJointBreakEvent(tBreakData);
	}
}

void CPhysxManagerListener::onWake(physx::PxActor** actors, physx::PxU32 count)
{
	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (!pPhysXManager)
		return;

    for (uint32_t i = 0; i < count; ++i)
    {
        physx::PxActor* pActor = actors[i];
		if (!pActor)
			continue;

		const auto userData = pPhysXManager->FindActorUserData(pActor);
		if (userData)
			PushEvent(EVENT_TYPE::WAKE, userData->hGameObject);
    }
}

void CPhysxManagerListener::onSleep(physx::PxActor** actors, physx::PxU32 count)
{
	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (!pPhysXManager)
		return;

    for (uint32_t i = 0; i < count; ++i)
    {
        physx::PxActor* pActor = actors[i];
		if (!pActor)
			continue;

		const auto userData = pPhysXManager->FindActorUserData(pActor);
		if (userData)
			PushEvent(EVENT_TYPE::SLEEP, userData->hGameObject);
    }
}

void CPhysxManagerListener::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
{
	// 헤더 레벨에서 이번 프레임 이전에 해제(Release)된 액터가 포함되어 있다면 통째로 스킵
	if (pairHeader.flags & (physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | physx::PxContactPairHeaderFlag::eREMOVED_ACTOR_1))
	{
		return;
	}

	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (!pPhysXManager || !pairHeader.actors[0] || !pairHeader.actors[1])
		return;

	const auto userDataA = pPhysXManager->FindActorUserData(pairHeader.actors[0]);
	const auto userDataB = pPhysXManager->FindActorUserData(pairHeader.actors[1]);
	if (!userDataA || !userDataB)
		return;

	for (physx::PxU32 i = 0; i < nbPairs; ++i)
    {
        const physx::PxContactPair& cp = pairs[i];
		// 개별 충돌 쌍(Pair) 중에 제거된 셰이프가 있다면 연산에서 제외
		if (cp.flags & (physx::PxContactPairFlag::eREMOVED_SHAPE_0 | physx::PxContactPairFlag::eREMOVED_SHAPE_1))
		{
			continue;
		}

		const auto shapeDataA = pPhysXManager->FindShapeUserData(cp.shapes[0]);
		const auto shapeDataB = pPhysXManager->FindShapeUserData(cp.shapes[1]);
		if (!shapeDataA || !shapeDataB)
			continue;

		PX_ON_COLLISION_DATA tData{};
		tData.eSelfShapeType = shapeDataA->eType;
		tData.eOtherShapeType = shapeDataB->eType;
		tData.iSelfShapeSubIndex = shapeDataA->iSubIndex;
		tData.iOtherShapeSubIndex = shapeDataB->iSubIndex;

		if (cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{
			physx::PxContactPairPoint contactPoints[PX_MAX_CONTACT_POINTS]{};
			const physx::PxU32 iContactCount = cp.extractContacts(contactPoints, PX_MAX_CONTACT_POINTS);
			tData.iContactCount = iContactCount;
			for (physx::PxU32 contactIndex = 0; contactIndex < iContactCount; ++contactIndex)
			{
				const auto& source = contactPoints[contactIndex];
				auto& destination = tData.Contacts[contactIndex];
				destination.vWorldPosition = { source.position.x, source.position.y, source.position.z };
				destination.vWorldNormal = { source.normal.x, source.normal.y, source.normal.z };
				destination.vImpulse = { source.impulse.x, source.impulse.y, source.impulse.z };
				destination.fSeparation = source.separation;
			}
			PushCollisionEvent(EVENT_TYPE::COLLISION_ENTER, userDataA->hGameObject, userDataB->hGameObject, tData);
		}
		if (cp.events & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
			PushCollisionEvent(EVENT_TYPE::COLLISION_EXIT, userDataA->hGameObject, userDataB->hGameObject, tData);
    }
}

void CPhysxManagerListener::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
{
	auto* pPhysXManager = CGameInstance::Get().GetPhysXManager();
	if (!pPhysXManager)
		return;

    for (physx::PxU32 i = 0; i < count; ++i)
    {
        const physx::PxTriggerPair& tp = pairs[i];

		// 트리거 이벤트 중 이미 삭제된 액터가 관여하고 있다면 안전하게 패스
		if (tp.flags & (physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
		{
			continue;
		}

        if (!tp.triggerActor || !tp.otherActor)
        {
            continue;
        }

		const auto userDataA = pPhysXManager->FindActorUserData(tp.triggerActor);
		const auto userDataB = pPhysXManager->FindActorUserData(tp.otherActor);
		const auto shapeDataA = pPhysXManager->FindShapeUserData(tp.triggerShape);
		const auto shapeDataB = pPhysXManager->FindShapeUserData(tp.otherShape);
		if (!userDataA || !userDataB || !shapeDataA || !shapeDataB)
			continue;

		PX_ON_TRIGGER_DATA tData{};
		tData.bSelfIsTrigger = true;
		tData.eSelfShapeType = shapeDataA->eType;
		tData.eOtherShapeType = shapeDataB->eType;
		tData.iSelfShapeSubIndex = shapeDataA->iSubIndex;
		tData.iOtherShapeSubIndex = shapeDataB->iSubIndex;
        
        if (tp.status & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND)
			PushTriggerEvent(EVENT_TYPE::TRIGGER_ENTER, userDataA->hGameObject, userDataB->hGameObject, tData);
		if (tp.status & physx::PxPairFlag::eNOTIFY_TOUCH_LOST)
			PushTriggerEvent(EVENT_TYPE::TRIGGER_EXIT, userDataA->hGameObject, userDataB->hGameObject, tData);
    }
}

void CPhysxManagerListener::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count)
{
    MSG_BOX("onAdvance 미구현");
}

UPtr<CPhysxManagerListener> CPhysxManagerListener::Create()
{
    auto pInstance = ToUPtr(new CPhysxManagerListener{ });
    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CPhysxManagerListener");
        return nullptr;
    }
    return pInstance;
}
