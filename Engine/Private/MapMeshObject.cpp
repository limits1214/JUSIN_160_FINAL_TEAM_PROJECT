#include "pch.h"
#include "MapMeshObject.h"
#include "ComConstantBuffer.h"
#include "ComStaticModelInstance.h"
#include "ComModelInstance.h"
#include "GameInstance.h"
#include "MapMeshGpuCuller.h"
#include "Resources.h"

NS_USING(Engine)

std::unordered_map<SPtr<CResStaticModel>, MAPMESH_INSTANCE_BATCH> CMapMeshObject::s_InstanceBatches{};
SPtr<CResDynamicBuffer> CMapMeshObject::s_pInstanceBuffer{};
SPtr<CResStructuredBuffer> CMapMeshObject::s_pOcclusionInputBuffer{};
SPtr<CResStructuredBuffer> CMapMeshObject::s_pVisibleFlagBuffer{};
size_t CMapMeshObject::s_iInstanceCapacity = 0;
std::optional<CHandle> CMapMeshObject::s_hRenderRepresentative = {};
UPtr<CMapMeshGpuCuller> CMapMeshObject::s_pGpuCuller{};
_bool CMapMeshObject::s_bInstancingEnabled = true;
_bool CMapMeshObject::s_bDebugBoundsEnabled = false;
CMapMeshObject::INSTANCING_STATS CMapMeshObject::s_FrameStats{ true };
CMapMeshObject::INSTANCING_STATS CMapMeshObject::s_LastStats{ true };

namespace
{
	SPtr<CResTexture2D> GetMapMeshTexture(const SPtr<CResStaticModel>& pModel, uint32_t meshIndex, AI_TEXTURE_TYPE materialType)
	{
		if (pModel == nullptr)
		{
			return nullptr;
		}

		auto& meshes = pModel->GetMeshes();
		if (meshIndex >= meshes.size() || meshes[meshIndex] == nullptr)
		{
			return nullptr;
		}

		auto& materials = pModel->GetMaterials();
		const uint32_t materialIndex = meshes[meshIndex]->Get_MaterialIndex();
		if (materialIndex >= materials.size() || materials[materialIndex] == nullptr)
		{
			return nullptr;
		}

		auto textures = materials[materialIndex]->GetTextures();
		if (textures[materialType].empty())
		{
			return nullptr;
		}

		return textures[materialType].front();
	}

	HRESULT BindMapMeshTextures(ID3D11DeviceContext* pContext, const SPtr<CResStaticModel>& pModel, uint32_t meshIndex)
	{
		if (pContext == nullptr)
		{
			return E_FAIL;
		}

		SPtr<CResTexture2D> diffuseTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE");
		if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE))
		{
			diffuseTexture = texture;
		}

		SPtr<CResTexture2D> normalTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL");
		if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_NORMALS))
		{
			normalTexture = texture;
		}

		SPtr<CResTexture2D> smroTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_SMRO");
		if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_METALNESS))
		{
			smroTexture = texture;
		}

		SPtr<CResTexture2D> emissiveTexture = CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_EMISSIVE");
		if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_EMISSIVE))
		{
			emissiveTexture = texture;
		}

		if (diffuseTexture == nullptr || normalTexture == nullptr || smroTexture == nullptr || emissiveTexture == nullptr)
		{
			return E_FAIL;
		}

		pContext->PSSetShaderResources(0, 1, diffuseTexture->GetSRV().GetAddressOf());
		pContext->PSSetShaderResources(1, 1, normalTexture->GetSRV().GetAddressOf());
		pContext->PSSetShaderResources(2, 1, smroTexture->GetSRV().GetAddressOf());
		pContext->PSSetShaderResources(3, 1, emissiveTexture->GetSRV().GetAddressOf());

		return S_OK;
	}

	HRESULT BindMapMeshMaterial(ID3D11DeviceContext* pContext, _float3 emissiveColor, _float emissiveIntensity, _float objectAlpha)
	{
		if (pContext == nullptr)
		{
			return E_FAIL;
		}

		auto materialConstantBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL");
		if (materialConstantBuffer == nullptr)
		{
			return E_FAIL;
		}

		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(pContext->Map(materialConstantBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
		{
			return E_FAIL;
		}

		CB_MATERIAL material{};
		material.EmissiveColor = emissiveColor;
		material.EmissiveIntensity = emissiveIntensity;
		material.ObjectAlpha = objectAlpha;

		memcpy(mapped.pData, &material, sizeof(CB_MATERIAL));
		pContext->Unmap(materialConstantBuffer->GetCBuffer().Get(), 0);
		pContext->PSSetConstantBuffers(3, 1, materialConstantBuffer->GetCBuffer().GetAddressOf());

		return S_OK;
	}
}

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

	if (s_bDebugBoundsEnabled)
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

	++s_FrameStats.iObjects;

	// ------------------------------------------- 인스턴싱 OFF --------------------------------------
	if (!s_bInstancingEnabled)
	{
		//if (CGameInstance::Get().IsOcclusionCulled(this))
		//{
		//	return;
		//}

		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
		return;
	}
	//------------------------------------------------------------------------------------------------



	// ------------------------------------------- 인스턴싱 ON --------------------------------------
	MAPMESH_INSTANCE_DATA instanceData{};
	MAPMESH_OCCLUSION_DATA occlusionData{};
	XMStoreFloat4x4(&instanceData.world, GetTransform().GetLoadedCombinedWorldMatrix());

	BoundingBox boundingBox;
	if (!GetOcclusionBounds(boundingBox))
		return;
	occlusionData.worldCenter = boundingBox.Center;
	occlusionData.worldExtents = boundingBox.Extents;
	PushInstance(m_pComModelInstance->GetModel(), instanceData, occlusionData);

	// 대표오브젝트 렌더러에 등록
	if (!s_hRenderRepresentative.has_value())
	{
		s_hRenderRepresentative = GetHandle();
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
	}
	//------------------------------------------------------------------------------------------------
}

HRESULT CMapMeshObject::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	// ------------------------------------------- 인스턴싱 OFF --------------------------------------
	if (!s_bInstancingEnabled)
	{
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
			pContext->VSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
			pContext->PSSetConstantBuffers(0, 1, m_pComCBufferPerObject->GetAdressOfBuffer());
		}

		auto pModel = m_pComModelInstance->GetModel();
		if (nullptr == pModel)	return E_FAIL;
		const uint32_t iNumMeshes = pModel->Get_NumMeshes();

		if (m_pSubMeshGpuCuller == nullptr)
		{
			m_pSubMeshGpuCuller = CMapMeshGpuCuller::Create();
		}

		if (m_pSubMeshGpuCuller != nullptr && iNumMeshes > 0)
		{
			std::vector<MAPMESH_OCCLUSION_DATA> subMeshBounds(iNumMeshes);
			const XMMATRIX world = GetTransform().GetLoadedCombinedWorldMatrix();

			for (uint32_t i = 0; i < iNumMeshes; ++i)
			{
				const auto& mesh = pModel->GetMeshes()[i];
				if (mesh == nullptr)
					continue;

				const auto& minPos = mesh->GetMinPos();
				const auto& maxPos = mesh->GetMaxPos();
				const BoundingBox localBounds{
					{ (minPos.x + maxPos.x) * 0.5f,
					  (minPos.y + maxPos.y) * 0.5f,
					  (minPos.z + maxPos.z) * 0.5f },
					{ (maxPos.x - minPos.x) * 0.5f,
					  (maxPos.y - minPos.y) * 0.5f,
					  (maxPos.z - minPos.z) * 0.5f }
				};

				BoundingBox worldBounds{};
				localBounds.Transform(worldBounds, world);
				subMeshBounds[i].worldCenter = worldBounds.Center;
				subMeshBounds[i].worldExtents = worldBounds.Extents;
				subMeshBounds[i].instanceIndex = i;
			}

			m_pSubMeshGpuCuller->BuildSubMeshVisibility(
				pContext,
				subMeshBounds,
				CGameInstance::Get().GetPrevHizBuffer(),
				ctx.matViewProj,
				CGameInstance::Get().GetClientScreenSize(),
				m_SubMeshVisibility);
		}

		for (uint32_t i = 0; i < iNumMeshes; ++i) {
			const auto& viBuffer = pModel->GetMeshes()[i];
			if (viBuffer == nullptr)
				continue;
			if (m_SubMeshVisibility.size() == iNumMeshes &&
				m_SubMeshVisibility[i] == 0)
			{
				continue;
			}

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
			++s_FrameStats.iDrawCalls;
		}
		

		return S_OK;
	}
	//------------------------------------------------------------------------------------------------


	// ------------------------------------------- 인스턴싱 ON --------------------------------------
	// 대표오브젝트만 렌더콜(DrawIndexedInstanced)
	if (!s_hRenderRepresentative.has_value() ||
		s_hRenderRepresentative.value() != GetHandle())
	{
		return S_OK;
	}

	return RenderInstancedBatches(pContext, ctx);
}

void CMapMeshObject::UpdateGUI()
{
	CGameObject::UpdateGUI();
}

bool CMapMeshObject::IsOcclusionCullable() const
{
	return !s_bInstancingEnabled &&
		m_pComModelInstance != nullptr &&
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
	m_SubMeshVisibility.clear();
	m_pSubMeshGpuCuller.reset();

	return S_OK;
}

void CMapMeshObject::SetInstancingEnabled(_bool bEnabled)
{
	if (s_bInstancingEnabled == bEnabled)
	{
		return;
	}

	s_bInstancingEnabled = bEnabled;
	ClearInstancingData();
}

void CMapMeshObject::ClearInstancingData()
{
	s_FrameStats.bEnabled = s_bInstancingEnabled;
	s_FrameStats.iInstances = 0;
	for (const auto& [pModel, instancesBatch] : s_InstanceBatches)
	{
		s_FrameStats.iInstances += static_cast<uint32_t>(instancesBatch.instances.size());
	}
	s_FrameStats.iBatches = static_cast<uint32_t>(s_InstanceBatches.size());
	s_LastStats = s_FrameStats;
	s_FrameStats = {};
	s_FrameStats.bEnabled = s_bInstancingEnabled;

	s_InstanceBatches.clear();
	s_hRenderRepresentative.reset();
}

void CMapMeshObject::ReleaseInstancingResources()
{
	s_InstanceBatches.clear();
	s_hRenderRepresentative.reset();

	s_pInstanceBuffer.reset();
	s_iInstanceCapacity = 0;

	s_pOcclusionInputBuffer.reset();
	s_pVisibleFlagBuffer.reset();
	s_pGpuCuller.reset();
}

HRESULT CMapMeshObject::PushInstance(const SPtr<CResStaticModel>& pModel, const MAPMESH_INSTANCE_DATA& instanceData, MAPMESH_OCCLUSION_DATA& occlusionData)
{
	if (pModel == nullptr)
	{
		return E_FAIL;
	}

	auto& batch = s_InstanceBatches[pModel];

	occlusionData.instanceIndex = static_cast<uint32_t>(batch.instances.size());

	batch.instances.push_back(instanceData);
	batch.occlusionData.push_back(occlusionData);

	++s_FrameStats.iInstances;

	return S_OK;
}

HRESULT CMapMeshObject::EnsureInstanceResources(size_t instanceCount)
{
	if (instanceCount == 0)
		return S_OK;

	if (s_pInstanceBuffer &&
		s_pOcclusionInputBuffer &&
		s_pVisibleFlagBuffer &&
		s_iInstanceCapacity >= instanceCount)
	{
		return S_OK;
	}

	size_t newCapacity = std::max<size_t>(instanceCount, 256);
	while (newCapacity < instanceCount)
	{
		newCapacity *= 2;
	}

	// 인스턴싱 렌더링용 vertex instance buffer 생성
	{
		auto pBuffer = CResDynamicBuffer::Create();
		if (pBuffer == nullptr)
		{
			return E_FAIL;
		}

		CResDynamicBuffer::DESC bufferDesc{};
		bufferDesc.desc = {
			.ByteWidth = static_cast<UINT>(sizeof(MAPMESH_INSTANCE_DATA) * newCapacity),
			.Usage = D3D11_USAGE_DYNAMIC,
			.BindFlags = D3D11_BIND_VERTEX_BUFFER,
			.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
			.MiscFlags = 0,
			.StructureByteStride = 0,
		};

		if (FAILED(pBuffer->Load(bufferDesc)))
		{
			return E_FAIL;
		}

		s_pInstanceBuffer = pBuffer;
	}

	{
		auto pBuffer = CResStructuredBuffer::Create();
		if (pBuffer == nullptr)
		{
			return E_FAIL;
		}

		CResStructuredBuffer::DESC bufferDesc{};
		bufferDesc.iNumElements = static_cast<uint32_t>(newCapacity);
		bufferDesc.iStructureByteStride = sizeof(MAPMESH_OCCLUSION_DATA);
		bufferDesc.pInitialData = nullptr;
		bufferDesc.bAppendConsume = false;

		if (FAILED(pBuffer->Load(bufferDesc)))
		{
			return E_FAIL;
		}

		s_pOcclusionInputBuffer = pBuffer;
	}

	{
		auto pBuffer = CResStructuredBuffer::Create();
		if (pBuffer == nullptr)
		{
			return E_FAIL;
		}

		CResStructuredBuffer::DESC bufferDesc{};
		bufferDesc.iNumElements = static_cast<uint32_t>(newCapacity);
		bufferDesc.iStructureByteStride = sizeof(uint32_t);
		bufferDesc.pInitialData = nullptr;
		bufferDesc.bAppendConsume = false;

		if (FAILED(pBuffer->Load(bufferDesc)))
		{
			return E_FAIL;
		}

		s_pVisibleFlagBuffer = pBuffer;
	}

	s_iInstanceCapacity = newCapacity;
	return S_OK;
}

HRESULT CMapMeshObject::RenderInstancedBatches(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	ZoneScopedN("RenderInstancedBatches");
	if (s_InstanceBatches.empty())
	{
		return S_OK;
	}


	const auto& vertexShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim_Instanced");
	const auto& pixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim_Instanced");
	const auto& sampler = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);

	if (vertexShader == nullptr || pixelShader == nullptr || sampler == nullptr)
	{
		return E_FAIL;
	}

	pContext->IASetInputLayout(vertexShader->GetInputLayout().Get());
	pContext->VSSetShader(vertexShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(pixelShader->GetPixelShader().Get(), nullptr, 0);

	for (auto& [pModel, instanceBatch] : s_InstanceBatches)
	{
		if (pModel == nullptr || instanceBatch.instances.empty())
		{
			continue;
		}

		if (s_pGpuCuller == nullptr)
		{
			s_pGpuCuller = CMapMeshGpuCuller::Create();
			if (s_pGpuCuller == nullptr)
			{
				return E_FAIL;
			}
		}

		if (FAILED(s_pGpuCuller->BuildVisibleInstances(
			pContext,
			instanceBatch.instances,
			instanceBatch.occlusionData,
			CGameInstance::Get().GetPrevHizBuffer(),
			ctx.matViewProj,
			CGameInstance::Get().GetClientScreenSize())))
		{
			return E_FAIL;
		}

#ifdef _DEBUG
		// 디버그 확인용!! cpu readback이라 병목 발생
		//{
		//	const uint32_t visibleInstanceCount = s_pGpuCuller->GetVisibleCountForDebug(pContext);
		//	const uint32_t totalInstanceCount = static_cast<uint32_t>(instanceBatch.instances.size());
		//	s_FrameStats.iVisibleInstances += visibleInstanceCount;
		//	s_FrameStats.iCulledInstances += totalInstanceCount - std::min(visibleInstanceCount, totalInstanceCount);
		//}
#endif

		ID3D11Buffer* visibleInstanceBuffer = s_pGpuCuller->GetVisibleInstanceBuffer();
		if (visibleInstanceBuffer == nullptr)
		{
			return E_FAIL;
		}

		const uint32_t numMeshes = pModel->Get_NumMeshes();
		for (uint32_t i = 0; i < numMeshes; ++i)
		{
			const auto& viBuffer = pModel->GetMeshes()[i];
			if (viBuffer == nullptr)
			{
				continue;
			}

			ID3D11Buffer* vertexBuffers[] = {
				viBuffer->GetVertexBuffer().Get(),
				visibleInstanceBuffer
			};
			uint32_t strides[] = {
				viBuffer->GetVertexStride(),
				static_cast<uint32_t>(sizeof(MAPMESH_INSTANCE_DATA)),
			};
			uint32_t offsets[] = { 0, 0 };

			pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
			pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
			pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

			if (FAILED(BindMapMeshTextures(pContext, pModel, i)))
			{
				return E_FAIL;
			}

			if (FAILED(BindMapMeshMaterial(pContext, { 1.f, 1.f, 1.f }, 0.f, 1.f)))
			{
				return E_FAIL;
			}

			

			if (FAILED(s_pGpuCuller->PrepareIndirectArgs(
				pContext,
				static_cast<uint32_t>(viBuffer->GetNumIndices()),
				0,
				0,
				0)))
			{
				return E_FAIL;
			}

			pContext->DrawIndexedInstancedIndirect(s_pGpuCuller->GetIndirectArgsBuffer(), 0);

			++s_FrameStats.iDrawCalls;
		}
	}

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
