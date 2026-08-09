#include "pch.h"
#include "MapMeshObject.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "ComModelInstance.h"
#include "GameInstance.h"
#include "MapMeshGpuCuller.h"
#include "Resources.h"
#include "CollBox.h"

NS_USING(Engine)

CMapMeshObject::CMapMeshObject()
	: CGameObject{}
{
}

CMapMeshObject::CMapMeshObject(const CMapMeshObject& Prototype)
	: CGameObject{ Prototype }
	, m_pResVertexShader{ Prototype.m_pResVertexShader }
	, m_pResPixelShader{ Prototype.m_pResPixelShader }
{
}

CMapMeshObject::~CMapMeshObject()
{
}

HRESULT CMapMeshObject::InitializePrototype(void* pArg)
{
	m_pResVertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim");
	if (!m_pResVertexShader || FAILED(m_pResVertexShader->Load()))
	{
		return E_FAIL;
	}

	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim");
	if (!m_pResPixelShader || FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CMapMeshObject::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}

	auto* pDesc = static_cast<MAP_MESH_OBJECT_DESC*>(pArg);
	if (pDesc == nullptr)
	{
		return E_FAIL;
	}

	m_modelResourceGroup = pDesc->modelGroupTag;
	m_modelResourceTag = pDesc->modelResTag;

	{
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerObject", &Desc, &m_pComCBufferPerObject)))
		{
			return E_FAIL;
		}
	}

	{
		CComStaticModelInstance::DESC Desc{};
		Desc.sGroupTag = m_modelResourceGroup;
		Desc.sResTag = m_modelResourceTag;
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_StaticModelInstance", "ComCModelInstance", &Desc, &m_pComModelInstance)))
		{
			return E_FAIL;
		}
	}

	m_WindDesc = pDesc->windDesc;

	return S_OK;
}

void CMapMeshObject::LateUpdate(_float fTimeDelta)
{
	GetTransform().Update();

	// 컬링된 애면 렌더러 등록x
	if (m_bRenderEnable == false)
		return;

	m_bRenderEnable = false;

	if (m_pComModelInstance == nullptr || m_pComModelInstance->GetModel() == nullptr)
	{
		return;
	}

	if (CGameInstance::Get().IsDebugBoundsEnabled())
	{
		BoundingBox debugBounds{};
		if (GetOcclusionBounds(debugBounds))
		{
			auto* pDebugLine = CGameInstance::Get().GetDbgLineRender();
			if (pDebugLine != nullptr)
			{
				pDebugLine->SetColor({ 1.f, 1.f, 0.f, 1.f });
				pDebugLine->AddBox(debugBounds.Extents, XMMatrixTranslation(
					debugBounds.Center.x, debugBounds.Center.y, debugBounds.Center.z));
				pDebugLine->SetColor();
			}
		}
	}

	if (!CGameInstance::Get().IsInstancingEnabled())
	{
		// ------------------------------------------- 인스턴싱 OFF --------------------------------------
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
		return;
		//------------------------------------------------------------------------------------------------
	}
	else
	{
		// ------------------------------------------- 인스턴싱 ON --------------------------------------
		// 렌더 큐에 넣지않고 instance데이터 push
		MAPMESH_INSTANCE_DATA instanceData{};
		MAPMESH_OCCLUSION_DATA occlusionData{};
		XMStoreFloat4x4(&instanceData.world, GetTransform().GetLoadedCombinedWorldMatrix());
		instanceData.windParams = {
			m_WindDesc.strength,
			m_WindDesc.speed,
			m_WindDesc.frequency,
			m_WindDesc.bendExponent
		};

		const auto& model = m_pComModelInstance->GetModel();
		if (model != nullptr && model->HasLocalBounds())
		{
			const BoundingBox& localBounds = model->GetLocalBounds();
			const _float modelMinY = localBounds.Center.y - localBounds.Extents.y;
			const _float modelHeight = localBounds.Extents.y * 2.f;
			const _float heightStart = std::clamp(m_WindDesc.heightStart, 0.f, 1.f);
			const _float heightEnd = std::clamp(m_WindDesc.heightEnd, heightStart, 1.f);
			const _float influenceHeight = modelHeight * (heightEnd - heightStart);

			if (influenceHeight > 0.0001f)
			{
				instanceData.windHeightParams = {
					modelMinY + modelHeight * heightStart,
					1.f / influenceHeight
				};
			}
		}
		instanceData.windType = static_cast<uint32_t>(m_WindDesc.type);
		const auto renderFeature = (instanceData.windType == static_cast<uint32_t>(EWindType::None))
			? EMapMeshRenderFeature::Static : EMapMeshRenderFeature::Foliage;

		BoundingBox boundingBox;
		if (!GetOcclusionBounds(boundingBox))
			return;
		occlusionData.worldCenter = boundingBox.Center;
		occlusionData.worldExtents = boundingBox.Extents;
		CGameInstance::Get().PushMapObjectInstance(m_pComModelInstance->GetModel(), instanceData, occlusionData, renderFeature);
		//------------------------------------------------------------------------------------------------
	}
}

HRESULT CMapMeshObject::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	// 인스턴싱 ON이면 개별 오브젝트에서 Render하지않는다.
	if (CGameInstance::Get().IsInstancingEnabled())
		return S_OK;

	// ------------------------------------------- 인스턴싱 OFF --------------------------------------
	if (m_pComModelInstance == nullptr || m_pComModelInstance->GetModel() == nullptr)
	{
		return E_FAIL;
	}

	{
		pContext->IASetInputLayout(m_pResVertexShader->GetInputLayout().Get());
		pContext->VSSetShader(m_pResVertexShader->GetVertexShader().Get(), nullptr, 0);
		pContext->PSSetShader(m_pResPixelShader->GetPixelShader().Get(), nullptr, 0);
	}

	{
		CB_PER_OBJECT cbPerObject{};
		cbPerObject.matWorld = *GetTransform().GetCombinedWorldMatrix();
		XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedCombinedWorldMatrix() * ctx.matViewProj);
		if (FAILED(m_pComCBufferPerObject->MapDiscard(pContext, &cbPerObject, sizeof(cbPerObject))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, m_pComCBufferPerObject->GetAdressOfBuffer());
	}

	auto pModel = m_pComModelInstance->GetModel();
	if (nullptr == pModel)	return E_FAIL;
	const uint32_t iNumMeshes = pModel->Get_NumMeshes();
	for (uint32_t i = 0; i < iNumMeshes; ++i) {
		const auto& viBuffer = pModel->GetMeshes()[i];

		ID3D11Buffer* vertexBuffers[] = { viBuffer->GetVertexBuffer().Get() };
		uint32_t strides[] = { viBuffer->GetVertexStride() };
		uint32_t offsets[] = { 0 };

		pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
		pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

		/*----------- 광윤 추가 -----------*/
		m_pComModelInstance->Bind_Textures(pContext, i);
		m_pComModelInstance->Bind_Materials(pContext, { 1.f, 1.f, 1.f }, 0.f, { 1.f, 1.f, 1.f }, 0.f, 1.f);	// EmissiveColor -> EmissiveIntensity -> Alpha 순
		/*---------------------------------*/

		pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);
	}
	return S_OK;
	//------------------------------------------------------------------------------------------------
}

/*----------- 광윤 추가 -----------*/
HRESULT CMapMeshObject::Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) {
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

bool CMapMeshObject::GetShadowBounds(BoundingBox& OutBounds) const {
	return GetOcclusionBounds(OutBounds);
}
/*---------------------------------*/

void CMapMeshObject::UpdateGUI()
{
	CGameObject::UpdateGUI();
}

bool CMapMeshObject::IsOcclusionCullable() const
{
	return m_pComModelInstance != nullptr &&
			m_pComModelInstance->GetModel() != nullptr;
}

bool CMapMeshObject::GetOcclusionBounds(BoundingBox& outBounds) const
{
	if (m_pComModelInstance == nullptr || m_pComModelInstance->GetModel() == nullptr)
		return false;

	const auto model = m_pComModelInstance->GetModel();
	if (!model->HasLocalBounds())
		return false;

	model->GetLocalBounds().Transform(outBounds, GetTransform().GetLoadedCombinedWorldMatrix());

	return true;
}

HRESULT CMapMeshObject::SetModelResource(const std::string& modelGroupTag, const std::string& modelResTag)
{
	if (m_pComModelInstance == nullptr)
	{
		return E_FAIL;
	}

	if (FAILED(m_pComModelInstance->ChangeModel(modelGroupTag, modelResTag)))
	{
		return E_FAIL;
	}

	m_modelResourceGroup = modelGroupTag;
	m_modelResourceTag = modelResTag;

	return S_OK;
}

UPtr<CMapMeshObject> CMapMeshObject::Create()
{
	auto pInstance = ToUPtr(new CMapMeshObject{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CMapMeshObject");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CMapMeshObject::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CMapMeshObject{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMapMeshObject");
		return nullptr;
	}

	return pInstance;
}
