#include "pch.h"
#include "OilBarrel.h"

#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComPxConvexCollider.h"
#include "ComPxDistanceJoint.h"
#include "ComPxFixedJoint.h"
#include "ComPxRigidBody.h"
#include "ComStaticModelInstance.h"
#include "GameInstance.h"
#include "ResPhysXConvexGeometry.h"
#include "ResPhysXMaterial.h"
#include "Resources.h"

NS_USING(Client)

namespace
{
	constexpr char TEST_DYNAMIC_CONVEX_PATH[] =
		"./Resources/PhysX/Cooked/SM_oil_barrel_0001.pxconvex";
}

COilBarrel::COilBarrel()
	: CGameObject{}
{
}

COilBarrel::COilBarrel(const COilBarrel& prototype)
	: CGameObject{ prototype }
	, m_pResVertexShader{ prototype.m_pResVertexShader }
	, m_pResPixelShader{ prototype.m_pResPixelShader }
{
}

HRESULT COilBarrel::InitializePrototype(void* pArg)
{
	//m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(
	//	TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	//if (!m_pResVertexShader || FAILED(m_pResVertexShader->Load()))
	//	return E_FAIL;

	//m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(
	//	TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	//if (!m_pResPixelShader || FAILED(m_pResPixelShader->Load()))
	//	return E_FAIL;

	return S_OK;
}

HRESULT COilBarrel::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	const auto* desc = static_cast<DESC*>(pArg);
	GetTransform().SetPosition(desc->vInitialPosition);
	GetTransform().SetRotationEuler(desc->vInitialRotation);
	GetTransform().SetScale(desc->vInitialScale);

	{
		CComConstantBuffer::DESC bufferDesc{};
		bufferDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject",
			&bufferDesc, &m_pComCBufferPerObject)))
			return E_FAIL;
	}

	{
		CComStaticModelInstance::DESC modelDesc{};
		modelDesc.sGroupTag = LEVEL::TERRAIN;
		//"LEVEL_CREATURE", "Static_OilBarrel_Resource"
		modelDesc.sResTag = "Static_OilBarrel_Resource";
		if (FAILED(AddComponentFromProto(
			"PERMANENT", "Prototype_Component_StaticModelInstance", "ComModelInstance",
			&modelDesc, &m_pComModelInstance)))
			return E_FAIL;
	}

	{
		CComPxRigidBody::DESC rigidBodyDesc{};
		rigidBodyDesc.eType = CComPxRigidBody::TYPE::DYNAMIC;
		rigidBodyDesc.fMass = std::max(desc->fMass, 0.001f);
		rigidBodyDesc.vPosition = desc->vInitialPosition;
		rigidBodyDesc.vRotation = GetTransform().GetQuaternion();
		if (FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxRigidBody", "ComPxRigidBody",
			&rigidBodyDesc, &m_pComPxRigidBody)))
			return E_FAIL;
	}

	{
		auto convexResource = CGameInstance::Get()
			.GetOrCreateResourceByPath<CResPhysXConvexGeometry>(
				TEST_DYNAMIC_CONVEX_PATH,
				[]() { return CResPhysXConvexGeometry::CreateAndLoad(TEST_DYNAMIC_CONVEX_PATH); });
		if (!convexResource)
			return E_FAIL;

		CComPxConvexCollider::DESC colliderDesc{};
		colliderDesc.pComPxRigidBody = m_pComPxRigidBody;
		colliderDesc.pResConvex = std::move(convexResource);
		colliderDesc.pResMaterial = CResPhysXMaterial::CreateAndLoad({});
		colliderDesc.vScale = {
			std::max(std::abs(desc->vConvexScale.x), 0.001f),
			std::max(std::abs(desc->vConvexScale.y), 0.001f),
			std::max(std::abs(desc->vConvexScale.z), 0.001f)
		};
		colliderDesc.tFilter = desc->tFilter;
		if (!colliderDesc.pResMaterial || FAILED(AddComponentFromProto(
			"PHYSX", "Prototype_Component_ComPxConvexCollider", "ComPxConvexCollider",
			&colliderDesc, &m_pComPxConvexCollider)))
			return E_FAIL;
	}

	if (!m_pComPxRigidBody->SetGravityEnabled(true))
		return E_FAIL;

	return S_OK;
}

void COilBarrel::PriorityUpdate(E::_float fTimeDelta)
{
}

void COilBarrel::Update(E::_float fTimeDelta)
{

}

void COilBarrel::LateUpdate(E::_float fTimeDelta)
{
	UpdatePhysicData();
	GetTransform().Update();

	if (!m_pComModelInstance || !m_pComModelInstance->GetModel())
		return;

	/*----------- 광윤 추가 -----------*/
	CGameInstance::Get().AddShadowRenderGroup(ACTORTYPE::DYNAMIC, this);
	/*---------------------------------*/

	if (!CGameInstance::Get().IsInstancingEnabled())
	{
		
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
		return;
	}

	const auto& pModel = m_pComModelInstance->GetModel();
	if (!pModel->HasLocalBounds())
		return;

	MAPMESH_INSTANCE_DATA InstanceData{};
	XMStoreFloat4x4(
		&InstanceData.world,
		GetTransform().GetLoadedCombinedWorldMatrix());

	BoundingBox WorldBounds{};
	pModel->GetLocalBounds().Transform(
		WorldBounds,
		GetTransform().GetLoadedCombinedWorldMatrix());

	MAPMESH_OCCLUSION_DATA OcclusionData{};
	OcclusionData.worldCenter = WorldBounds.Center;
	OcclusionData.worldExtents = WorldBounds.Extents;

	CGameInstance::Get().PushMapObjectInstance(pModel, InstanceData, OcclusionData);
}

HRESULT COilBarrel::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject ||
		!m_pResVertexShader || !m_pResPixelShader)
		return E_FAIL;

	E::CB_PER_OBJECT cbPerObject{};
	cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(&cbPerObject.matWVP,
		GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(
		pContext, &cbPerObject, sizeof(cbPerObject))))
		return E_FAIL;
	pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());

	pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
	pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);

	const auto model = m_pComModelInstance->GetModel();
	if (!model)
		return E_FAIL;

	for (uint32_t i = 0; i < model->Get_NumMeshes(); ++i)
	{
		const auto& viBuffer = model->GetMeshes()[i];
		ID3D11Buffer* vertexBuffer = viBuffer->GetVertexBuffer().Get();
		const uint32_t stride = viBuffer->GetVertexStride();
		const uint32_t offset = 0;
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(
			viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		m_pComModelInstance->Bind_Textures(pContext, i);
		m_pComModelInstance->Bind_Materials(
			pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}

	return S_OK;
}

_bool COilBarrel::ApplyPushForce(const _float3& vDirection, _float fStrength)
{
	if (!m_pComPxRigidBody || fStrength <= 0.f)
		return false;

	const _vector vDirectionVector = XMLoadFloat3(&vDirection);
	if (XMVectorGetX(XMVector3LengthSq(vDirectionVector)) <= FLT_EPSILON)
		return false;

	_float3 vNormalizedDirection{};
	XMStoreFloat3(&vNormalizedDirection, XMVector3Normalize(vDirectionVector));
	return m_pComPxRigidBody->AddForce({
		vNormalizedDirection.x * fStrength,
		vNormalizedDirection.y * fStrength,
		vNormalizedDirection.z * fStrength });
}

_bool COilBarrel::OnAcquireFromPool(void* pArg)
{
	const auto* pDesc = static_cast<POOL_ACQUIRE_DESC*>(pArg);
	if (!pDesc || !m_pComPxRigidBody || !m_pComPxConvexCollider)
		return false;

	if (!m_pComPxRigidBody->SetPose(
			pDesc->vPosition,
			pDesc->vRotation) ||
		!m_pComPxRigidBody->SetLinearVelocity({}) ||
		!m_pComPxRigidBody->SetAngularVelocity({}))
	{
		return false;
	}

	GetTransform().SetPosition(pDesc->vPosition);
	GetTransform().SetQuaternion(pDesc->vRotation);
	GetTransform().Update();
	return true;
}

void COilBarrel::OnReleaseToPool()
{
	// [LSY] 재사용 전에 다른 객체와 맺은 Joint 관계를 모두 끊는다.
	if (m_pComPxRigidBody)
		m_pComPxRigidBody->ReleaseConnectedJoints();

	if (m_pComPxFixedJoint)
	{
		if (SUCCEEDED(DelComponent("ComPxFixedJoint")))
			m_pComPxFixedJoint = nullptr;
		else
			DEBUG_LOG("[OilBarrelPool] Failed to remove FixedJoint component.\n");
	}

	if (m_pComPxDistanceJoint)
	{
		if (SUCCEEDED(DelComponent("ComPxDistanceJoint")))
			m_pComPxDistanceJoint = nullptr;
		else
			DEBUG_LOG("[OilBarrelPool] Failed to remove DistanceJoint component.\n");
	}
}

void COilBarrel::OnManagedUpdateEnabled()
{
	if (!m_pComPxRigidBody || !m_pComPxConvexCollider)
		return;

	const _bool bSimulationEnabled =
		m_pComPxConvexCollider->SetSimulationEnabled(true);
	const _bool bQueryEnabled =
		m_pComPxConvexCollider->SetQueryEnabled(true);
	const _bool bGravityEnabled =
		m_pComPxRigidBody->SetGravityEnabled(true);
	const _bool bAwake = m_pComPxRigidBody->WakeUp();

	if (!bSimulationEnabled || !bQueryEnabled ||
		!bGravityEnabled || !bAwake)
	{
		DEBUG_LOG("[OilBarrelPool] Failed to enable pooled object physics.\n");
	}
}

void COilBarrel::OnManagedUpdateDisabled()
{
	if (!m_pComPxRigidBody || !m_pComPxConvexCollider)
		return;

	const _bool bLinearVelocityCleared =
		m_pComPxRigidBody->SetLinearVelocity({});
	const _bool bAngularVelocityCleared =
		m_pComPxRigidBody->SetAngularVelocity({});
	const _bool bSimulationDisabled =
		m_pComPxConvexCollider->SetSimulationEnabled(false);
	const _bool bQueryDisabled =
		m_pComPxConvexCollider->SetQueryEnabled(false);
	const _bool bGravityDisabled =
		m_pComPxRigidBody->SetGravityEnabled(false);
	const _bool bAsleep = m_pComPxRigidBody->PutToSleep();

	if (!bLinearVelocityCleared || !bAngularVelocityCleared ||
		!bSimulationDisabled || !bQueryDisabled ||
		!bGravityDisabled || !bAsleep)
	{
		DEBUG_LOG("[OilBarrelPool] Failed to disable pooled object physics.\n");
	}
}

_bool COilBarrel::CreateFixedJoint(
	COilBarrel* pConnectedBarrel,
	uint32_t iJointSubIndex)
{
	if (m_pComPxFixedJoint && !m_pComPxFixedJoint->IsValid())
	{
		if (SUCCEEDED(DelComponent("ComPxFixedJoint")))
			m_pComPxFixedJoint = nullptr;
	}

	if (!m_pComPxRigidBody || m_pComPxFixedJoint)
		return false;

	if (pConnectedBarrel &&
		(!pConnectedBarrel->m_pComPxRigidBody ||
		 pConnectedBarrel == this))
	{
		return false;
	}

	CComPxFixedJoint::DESC tJointDesc{};
	tJointDesc.pRigidBodyA = m_pComPxRigidBody;
	tJointDesc.pRigidBodyB = pConnectedBarrel
		? pConnectedBarrel->m_pComPxRigidBody
		: nullptr;
	tJointDesc.bPreserveCurrentPose = true;
	tJointDesc.bCollisionEnabled = false;
	tJointDesc.bVisualizationEnabled = true;
	tJointDesc.iJointSubIndex = iJointSubIndex;
	//tJointDesc.fBreakForce = 2.f;

	m_pComPxFixedJoint =
		CGameInstance::Get().AddPxJoint<CComPxFixedJoint>(
			*this,
			"ComPxFixedJoint",
			tJointDesc);

	return m_pComPxFixedJoint != nullptr;
}

_bool COilBarrel::CreateDistanceJoint(
	COilBarrel* pConnectedBarrel,
	_float fMaxDistance,
	uint32_t iJointSubIndex)
{
	if (m_pComPxDistanceJoint && !m_pComPxDistanceJoint->IsValid())
	{
		if (SUCCEEDED(DelComponent("ComPxDistanceJoint")))
			m_pComPxDistanceJoint = nullptr;
	}

	if (!m_pComPxRigidBody ||
		m_pComPxDistanceJoint ||
		!pConnectedBarrel ||
		pConnectedBarrel == this ||
		!pConnectedBarrel->m_pComPxRigidBody ||
		!std::isfinite(fMaxDistance) ||
		fMaxDistance <= 0.f)
	{
		return false;
	}

	CComPxDistanceJoint::DESC tJointDesc{};
	tJointDesc.pRigidBodyA = m_pComPxRigidBody;
	tJointDesc.pRigidBodyB =
		pConnectedBarrel->m_pComPxRigidBody;
	tJointDesc.fMinDistance = 0.f;
	tJointDesc.fMaxDistance = fMaxDistance;
	tJointDesc.fTolerance = 0.025f;
	tJointDesc.bMinDistanceEnabled = false;
	tJointDesc.bMaxDistanceEnabled = true;
	tJointDesc.bSpringEnabled = false;
	tJointDesc.bCollisionEnabled = false;
	tJointDesc.bVisualizationEnabled = true;
	tJointDesc.iJointSubIndex = iJointSubIndex;

	m_pComPxDistanceJoint =
		CGameInstance::Get().AddPxJoint<CComPxDistanceJoint>(
			*this,
			"ComPxDistanceJoint",
			tJointDesc);

	return m_pComPxDistanceJoint != nullptr;
}

void COilBarrel::OnJointBreak(
	const PX_ON_JOINT_BREAK_DATA& tData)
{
	DEBUG_LOG_STR(
		std::string{ "[PX][OilBarrel] Joint Broken. SubIndex: " } +
		std::to_string(tData.iJointSubIndex) +
		"\n");
}

/*----------- 광윤 추가 -----------*/
HRESULT COilBarrel::Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {
	if (!pContext || !m_pComModelInstance || !m_pComCBufferPerObject)
		return E_FAIL;

	E::CB_PER_OBJECT cbPerObject{};
	cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
	XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
	if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))	return E_FAIL;

	pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());

	const auto model = m_pComModelInstance->GetModel();
	if (!model)	return E_FAIL;

	for (uint32_t i = 0; i < model->Get_NumMeshes(); ++i)
	{
		const auto& viBuffer = model->GetMeshes()[i];
		ID3D11Buffer* vertexBuffer = viBuffer->GetVertexBuffer().Get();
		const uint32_t stride = viBuffer->GetVertexStride();
		const uint32_t offset = 0;

		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());
		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}

	ID3D11ShaderResourceView* pSRVs[4] = { nullptr, nullptr, nullptr, nullptr };
	pContext->PSSetShaderResources(0, 4, pSRVs);

	return S_OK;
}
bool COilBarrel::GetShadowBounds(BoundingBox& OutBounds) const{
	if (m_pComModelInstance == nullptr)	return false;

	const auto& Model =m_pComModelInstance->GetModel();
	if (Model == nullptr || !Model->HasLocalBounds())		return false;

	Model->GetLocalBounds().Transform(OutBounds, GetTransform().GetLoadedCombinedWorldMatrix());

	return true;
}
/*---------------------------------*/

E::UPtr<COilBarrel> COilBarrel::Create()
{
	auto instance = E::ToUPtr(new COilBarrel{});
	if (FAILED(instance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: COilBarrel");
		return nullptr;
	}
	return instance;
}

E::UPtr<E::CPrototype> COilBarrel::Clone(void* pArg)
{
	auto instance = E::ToUPtr(new COilBarrel{ *this });
	if (FAILED(instance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: COilBarrel");
		return nullptr;
	}
	return instance;
}
