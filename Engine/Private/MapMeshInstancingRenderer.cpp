#include "pch.h"
#include "MapMeshInstancingRenderer.h"
#include "MapMeshGpuCuller.h"
#include "MapMeshObject.h"

NS_USING(Engine)

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
}

const std::vector<CMapMeshInstancingRenderer::MAPMESH_TEXTURE_SET>*
CMapMeshInstancingRenderer::GetOrCreateMapMeshTextureCache(const SPtr<CResStaticModel>& pModel)
	{
		if (pModel == nullptr)
		{
			return nullptr;
		}

		if (const auto iter = m_MapMeshTextureCache.find(pModel); iter != m_MapMeshTextureCache.end())
		{
			return &iter->second;
		}

		MAPMESH_TEXTURE_SET defaultTextures{
			CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE"),
			CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL"),
			CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_SMRO"),
			CGameInstance::Get().GetResourceFirst<CResTexture2D>("DEFAULT_TEXTURE", "TEX_DEFAULT_EMISSIVE")
		};
		if (std::ranges::any_of(defaultTextures, [](const auto& texture) { return texture == nullptr; }))
		{
			return nullptr;
		}

		std::vector<MAPMESH_TEXTURE_SET> textureSets(pModel->Get_NumMeshes(), defaultTextures);
		for (uint32_t meshIndex = 0; meshIndex < textureSets.size(); ++meshIndex)
		{
			auto& textures = textureSets[meshIndex];
			if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_DIFFUSE))
				textures[0] = std::move(texture);
			if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_NORMALS))
				textures[1] = std::move(texture);
			if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_METALNESS))
				textures[2] = std::move(texture);
			if (auto texture = GetMapMeshTexture(pModel, meshIndex, AI_TEXTURE_TYPE::aiTextureType_EMISSIVE))
				textures[3] = std::move(texture);
		}

		auto [iter, inserted] = m_MapMeshTextureCache.emplace(pModel, std::move(textureSets));
		return &iter->second;
	}

HRESULT CMapMeshInstancingRenderer::BindMapMeshTextures(
	ID3D11DeviceContext* pContext,
	const std::vector<MAPMESH_TEXTURE_SET>& textureCache,
	uint32_t meshIndex) const
	{
		if (pContext == nullptr || meshIndex >= textureCache.size())
		{
			return E_FAIL;
		}

		ID3D11ShaderResourceView* srvs[MAPMESH_TEXTURE_COUNT]{};
		for (size_t i = 0; i < MAPMESH_TEXTURE_COUNT; ++i)
		{
			if (textureCache[meshIndex][i] == nullptr)
				return E_FAIL;
			srvs[i] = textureCache[meshIndex][i]->GetSRV().Get();
		}
		pContext->PSSetShaderResources(0, MAPMESH_TEXTURE_COUNT, srvs);

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
		pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::MATERIAL), 1, materialConstantBuffer->GetCBuffer().GetAddressOf());

		return S_OK;
	}


CMapMeshInstancingRenderer::CMapMeshInstancingRenderer()
{

}
CMapMeshInstancingRenderer::~CMapMeshInstancingRenderer()
{
	ReleaseInstancingResources();
}

HRESULT CMapMeshInstancingRenderer::Initialize()
{
	s_pGpuCuller = CMapMeshGpuCuller::Create();

	if (s_pGpuCuller == nullptr)
		return E_FAIL;


	return S_OK;
}

HRESULT CMapMeshInstancingRenderer::PushMapObjectInstance(const SPtr<CResStaticModel>& pModel, EMapMeshRenderFeature renderFeature, const MAPMESH_INSTANCE_DATA& instanceData, MAPMESH_OCCLUSION_DATA& occlusionData)
{
	if (pModel == nullptr)
	{
		return E_FAIL;
	}
	MAPOBJECTKEY key{ pModel, renderFeature };
	auto& batch = s_InstanceBatches[key];

	occlusionData.instanceIndex = static_cast<uint32_t>(batch.instances.size());

	batch.instances.push_back(instanceData);
	batch.occlusionData.push_back(occlusionData);

	if(s_bInstancingEnabled)
		++s_FrameStats.iInstances;

	return S_OK;
}

void CMapMeshInstancingRenderer::Update()
{
	// 인스턴싱 ON 일 때
	if (s_bInstancingEnabled)
		CGameInstance::Get().AddRenderObject(RENDERGROUP::NONBLEND, this);
}

void CMapMeshInstancingRenderer::FrameEnd()
{
	ClearInstancingData();
	ClearFrameScratchBuffers();
}

void CMapMeshInstancingRenderer::ClearTextureCache()
{
	m_MapMeshTextureCache.clear();
}

void CMapMeshInstancingRenderer::EraseTextureCache(const SPtr<CResStaticModel>& model)
{
	//auto iter = m_MapMeshTextureCache.find(model);
	//if (iter == m_MapMeshTextureCache.end())
	//{
	//	return;
	//}
	m_MapMeshTextureCache.erase(model);
}

HRESULT CMapMeshInstancingRenderer::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	ZoneScopedN("MapMeshInstancingRender");
	ClearFrameScratchBuffers();
	if (pContext == nullptr || s_InstanceBatches.empty())
		return S_OK;

	const auto& vertexStaticShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim_Instanced");
	const auto& vertexFoliageShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim_Instanced_Foliage");
	const auto& pixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim_Instanced");
	const auto& sampler = CGameInstance::Get().GetResourceFirst<CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
	if (!vertexStaticShader || !vertexFoliageShader || !pixelShader || !sampler)
		return E_FAIL;

	//pContext->IASetInputLayout(vertexStaticShader->GetInputLayout().Get());
	//pContext->VSSetShader(vertexStaticShader->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(pixelShader->GetPixelShader().Get(), nullptr, 0);
	pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());

	if (s_pGpuCuller == nullptr)
	{
		s_pGpuCuller = CMapMeshGpuCuller::Create();
		if (s_pGpuCuller == nullptr)
			return E_FAIL;
	}

	size_t totalInstances = 0;
	size_t totalDraws = 0;
	for (const auto& [pair, batch] : s_InstanceBatches)
	{
		const auto& model = pair.first;
		if (model && !batch.instances.empty())
		{
			totalInstances += batch.instances.size();
			totalDraws += model->Get_NumMeshes();
		}
	}

	m_Instances.reserve(totalInstances);
	m_OcclusionData.reserve(totalInstances);
	m_CullMeta.reserve(totalInstances);
	m_DrawBatchIndices.reserve(totalDraws);
	m_IndirectArgs.reserve(totalDraws);
	m_DrawItems.reserve(totalDraws);

	uint32_t batchIndex = 0;
	for (const auto& [pair, batch] : s_InstanceBatches)
	{
		const auto& model = pair.first;
		const auto& renderFeature = pair.second;
		if (!model || batch.instances.empty())
			continue;
		if (batch.occlusionData.size() != batch.instances.size())
			return E_FAIL;

		const auto* textureCache = GetOrCreateMapMeshTextureCache(model);
		if (textureCache == nullptr)
			return E_FAIL;

		const uint32_t instanceOffset = static_cast<uint32_t>(m_Instances.size());
		m_Instances.insert(m_Instances.end(), batch.instances.begin(), batch.instances.end());
		m_OcclusionData.insert(m_OcclusionData.end(), batch.occlusionData.begin(), batch.occlusionData.end());
		m_CullMeta.insert(m_CullMeta.end(), batch.instances.size(), MAPMESH_CULL_META{ instanceOffset, batchIndex });

		for (uint32_t meshIndex = 0; meshIndex < model->Get_NumMeshes(); ++meshIndex)
		{
			const auto& mesh = model->GetMeshes()[meshIndex];
			if (mesh == nullptr)
				continue;
			const uint32_t drawIndex = static_cast<uint32_t>(m_DrawItems.size());

			m_DrawBatchIndices.push_back(batchIndex);
			D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS args{};
			args.IndexCountPerInstance = static_cast<uint32_t>(mesh->GetNumIndices());
			m_IndirectArgs.push_back(args);
			m_DrawItems.push_back({ model, renderFeature, textureCache, meshIndex, instanceOffset });

			const size_t featureIndex = static_cast<size_t>(renderFeature);
			if (featureIndex >= RENDER_FEATURE_COUNT)
				return E_FAIL;
			m_DrawIndicesByFeature[featureIndex].push_back(drawIndex);
		}
		++batchIndex;
	}

	if (m_Instances.empty() || m_DrawItems.empty())
		return S_OK;

	if (FAILED(s_pGpuCuller->BuildVisibleInstancesAndIndirectArgs(
		pContext, m_Instances, m_OcclusionData, m_CullMeta, batchIndex,
		m_DrawBatchIndices, m_IndirectArgs,
		CGameInstance::Get().GetPrevHizBuffer(), ctx.matViewProj,
		CGameInstance::Get().GetClientScreenSize())))
	{
		return E_FAIL;
	}

	ID3D11Buffer* visibleInstanceBuffer = s_pGpuCuller->GetVisibleInstanceBuffer();
	ID3D11Buffer* argsBuffer = s_pGpuCuller->GetIndirectArgsBuffer();
	if (!visibleInstanceBuffer || !argsBuffer)
		return E_FAIL;
	if (FAILED(BindMapMeshMaterial(pContext, { 1.f, 1.f, 1.f }, 0.f, 1.f)))
		return E_FAIL;

	const auto RenderFeature = [&](EMapMeshRenderFeature renderFeature, const SPtr<CResVertexShader>& vertexShader) -> HRESULT
	{
		const size_t featureIndex = static_cast<size_t>(renderFeature);
		if (featureIndex >= RENDER_FEATURE_COUNT || vertexShader == nullptr)
			return E_FAIL;

		const auto& drawIndices = m_DrawIndicesByFeature[featureIndex];
		if (drawIndices.empty())
			return S_OK;

		pContext->IASetInputLayout(vertexShader->GetInputLayout().Get());
		pContext->VSSetShader(vertexShader->GetVertexShader().Get(), nullptr, 0);

		for (const uint32_t drawIndex : drawIndices)
		{
			if (drawIndex >= m_DrawItems.size())
				return E_FAIL;

			const auto& item = m_DrawItems[drawIndex];
			const auto& mesh = item.model->GetMeshes()[item.meshIndex];
			ID3D11Buffer* vertexBuffers[] = { mesh->GetVertexBuffer().Get(), visibleInstanceBuffer };
			uint32_t strides[] = { mesh->GetVertexStride(), static_cast<uint32_t>(sizeof(MAPMESH_INSTANCE_DATA)) };
			uint32_t offsets[] = { 0, item.instanceOffset * static_cast<uint32_t>(sizeof(MAPMESH_INSTANCE_DATA)) };

			pContext->IASetVertexBuffers(0, 2, vertexBuffers, strides, offsets);
			pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
			pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
			if (FAILED(BindMapMeshTextures(pContext, *item.textureCache, item.meshIndex)))
				return E_FAIL;

			pContext->DrawIndexedInstancedIndirect(
				argsBuffer,
				drawIndex * static_cast<uint32_t>(sizeof(D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS)));
			++s_FrameStats.iDrawCalls;
		}

		return S_OK;
	};

	if (FAILED(RenderFeature(EMapMeshRenderFeature::Static, vertexStaticShader)))
		return E_FAIL;
	if (FAILED(RenderFeature(EMapMeshRenderFeature::Foliage, vertexFoliageShader)))
		return E_FAIL;

	return S_OK;
}

bool CMapMeshInstancingRenderer::HasRenderPass(RENDERPASS ePass) const
{
	return ePass == RENDERPASS::DEFAULT;
}

void CMapMeshInstancingRenderer::SetInstancingEnabled(_bool bEnabled)
{
	if (s_bInstancingEnabled == bEnabled)
	{
		return;
	}

	s_bInstancingEnabled = bEnabled;
	ClearInstancingData();
}

void CMapMeshInstancingRenderer::ClearInstancingData()
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
}

void CMapMeshInstancingRenderer::ClearFrameScratchBuffers()
{
	m_Instances.clear();
	m_OcclusionData.clear();
	m_CullMeta.clear();
	m_DrawBatchIndices.clear();
	m_IndirectArgs.clear();
	m_DrawItems.clear();
	for (auto& drawIndices : m_DrawIndicesByFeature)
		drawIndices.clear();
}

void CMapMeshInstancingRenderer::ReleaseInstancingResources()
{
	s_InstanceBatches.clear();
	ClearFrameScratchBuffers();
	ClearTextureCache();
	s_pGpuCuller.reset();
}

UPtr<CMapMeshInstancingRenderer> CMapMeshInstancingRenderer::Create()
{
	auto pInstance = ToUPtr(new CMapMeshInstancingRenderer{});
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
	
}
void CMapMeshInstancingRenderer::Free()
{
	CEngineBase::Free();
}
