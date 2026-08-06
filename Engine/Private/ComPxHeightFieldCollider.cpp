#include "pch.h"
#include "ComPxHeightFieldCollider.h"

#include "ComPxRigidBody.h"
#include "ResPhysXHeightFieldGeometry.h"

#pragma push_macro("new")
#undef new

#include "PxPhysicsAPI.h"

#pragma pop_macro("new")

#include <cmath>

using namespace physx;

NS_USING(Engine)

void CComPxHeightFieldCollider::UpdateGUI()
{
	CComPxCollider::UpdateGUI();

	ImGui::PushID(this);
	ImGui::Text("Height Scale: %.8f", m_fHeightScale);
	ImGui::Text("Row Scale (X): %.8f", m_fRowScale);
	ImGui::Text("Column Scale (Z): %.8f", m_fColumnScale);
	ImGui::TextDisabled("HeightField trigger is not supported.");
	ImGui::Separator();
	ImGui::PopID();
}

CComPxHeightFieldCollider::CComPxHeightFieldCollider()
{
}

CComPxHeightFieldCollider::~CComPxHeightFieldCollider()
{
}

HRESULT CComPxHeightFieldCollider::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc)
		return E_FAIL;

	if (!pDesc->pResHeightField ||
		!std::isfinite(pDesc->fHeightScale) ||
		!std::isfinite(pDesc->fRowScale) ||
		!std::isfinite(pDesc->fColumnScale) ||
		pDesc->fHeightScale <= 0.f ||
		pDesc->fRowScale <= 0.f ||
		pDesc->fColumnScale <= 0.f)
	{
		DEBUG_LOG("[PX][HeightField] Invalid resource or scale.\n");
		return E_FAIL;
	}

	if (pDesc->bIsTrigger)
	{
		DEBUG_LOG("[PX][HeightField] Height-field triggers are not supported.\n");
		return E_FAIL;
	}

	if (FAILED(CComPxCollider::Initialize(pArg)))
		return E_FAIL;

	m_pResHeightField = pDesc->pResHeightField;
	m_fHeightScale = pDesc->fHeightScale;
	m_fRowScale = pDesc->fRowScale;
	m_fColumnScale = pDesc->fColumnScale;

	auto* pPhysics = CGameInstance::Get().PxGetPhysics();
	auto* pHeightField = m_pResHeightField->GetHeightField();
	auto* pMaterial = m_pResMaterial->GetMaterial();
	if (!pPhysics || !pHeightField || !pMaterial)
		return E_FAIL;

	auto* pActor = m_pComRigidBody->GetActor();
	if (!pActor)
		return E_FAIL;

	// 현재 엔진의 TriMesh 정책과 동일하게 일반 Dynamic에는 부착하지 않습니다.
	// Kinematic을 별도 TYPE으로 관리한다면 STATIC 또는 KINEMATIC만 허용하십시오.
	if (m_pComRigidBody->GetRigidBodyType() == CComPxRigidBody::TYPE::DYNAMIC)
	{
		DEBUG_LOG("[PX][HeightField] Height field requires a static or kinematic rigid body.\n");
		return E_FAIL;
	}

	const PxHeightFieldGeometry geometry{
		pHeightField,
		PxMeshGeometryFlags{},
		m_fHeightScale,
		m_fRowScale,
		m_fColumnScale
	};

	if (!geometry.isValid())
	{
		DEBUG_LOG("[PX][HeightField] Invalid PxHeightFieldGeometry.\n");
		return E_FAIL;
	}

	m_pShape = pPhysics->createShape(geometry, *pMaterial, true);
	if (!m_pShape)
	{
		DEBUG_LOG("[PX][HeightField] PxPhysics::createShape failed.\n");
		return E_FAIL;
	}

	if (pDesc->vLocalOffset.x != 0.f ||
		pDesc->vLocalOffset.y != 0.f ||
		pDesc->vLocalOffset.z != 0.f)
	{
		PxTransform tLocalPose = m_pShape->getLocalPose();
		tLocalPose.p += PxVec3{
			pDesc->vLocalOffset.x,
			pDesc->vLocalOffset.y,
			pDesc->vLocalOffset.z
		};
		m_pShape->setLocalPose(tLocalPose);
	}

	if (!pActor->attachShape(*m_pShape))
	{
		DEBUG_LOG("[PX][HeightField] Failed to attach shape.\n");
		return E_FAIL;
	}

	// PX_SHAPE_TYPE에 HEIGHT_FIELD 항목을 추가해야 합니다.
	if (!RegisterShape(PX_SHAPE_TYPE::HEIGHT_FIELD))
	{
		pActor->detachShape(*m_pShape);
		return E_FAIL;
	}

	return S_OK;
}

UPtr<CComPxHeightFieldCollider> CComPxHeightFieldCollider::Create()
{
	auto pInstance = ToUPtr(new CComPxHeightFieldCollider{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComPxHeightFieldCollider");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CComPxHeightFieldCollider::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CComPxHeightFieldCollider{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned: CComPxHeightFieldCollider");
		return nullptr;
	}
	return pInstance;
}

void CComPxHeightFieldCollider::Free()
{
	// Shape가 PxHeightField를 참조하므로 Shape를 먼저 해제합니다.
	CComPxCollider::Free();
	//m_pResHeightField.reset();
}
