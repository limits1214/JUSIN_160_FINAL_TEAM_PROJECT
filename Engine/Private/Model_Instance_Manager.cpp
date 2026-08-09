#include "pch.h"
#include "Model_Instance_Manager.h"

#include "ComAnimator.h"
#include "AnimationObject.h"
#include "ComModelInstance.h"
#include "ComStaticModelInstance.h"
#include "ResModel.h"

NS_USING(Engine)

CModel_Instance_Manager::CModel_Instance_Manager(): CEngineBase{}
{
}

CModel_Instance_Manager::~CModel_Instance_Manager()
{
	m_ActiveBatches.clear();
	m_InstanceBatches.clear();
}

HRESULT CModel_Instance_Manager::Initialize()
{
	m_InstanceBatches.clear();
	m_ActiveBatches.clear();

	m_iTotalInstanceCount = 0;

	m_pResSkinMeshCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_GPU_SKIN_MESH");
	if (!m_pResSkinMeshCBuffer)		return E_FAIL;

	m_ShadowFilteredInstances.reserve(MAX_INSTANCE_COUNT);
	m_ShadowVisibleSourceIndices.reserve(MAX_INSTANCE_COUNT);

	m_ShadowBonePaletteScratch.resize(static_cast<size_t>(MAX_INSTANCE_COUNT) * MAX_BONE_COUNT);

	return S_OK;
}

void CModel_Instance_Manager::Add_Instance(CComModelInstance* pModelInstance,CComAnimator* pAnimator,const _float4x4& WorldMatrix,uint32_t iFlags)
{
	if (!pModelInstance ||!pAnimator)
	{
		return;
	}

	const auto& pModel =pModelInstance->GetModel();

	if (!pModel)
		return;

	const auto& AnimState =pAnimator->GetCurAnimState();

	if (AnimState.iAnimIndex < 0)
		return;

	const uint32_t iAnimationCount = (uint32_t) pModel->GetAnimations().size();

	const uint32_t iAnimIndex = (uint32_t)AnimState.iAnimIndex;

	if (iAnimIndex >= iAnimationCount)
		return;

	GPU_ANIM_INSTANCE_DATA InstanceData{};
	InstanceData.WorldMatrix = WorldMatrix;
	InstanceData.iAnimIndex = iAnimIndex;
	InstanceData.iFlags = iFlags;
	InstanceData.fTrackPosition = AnimState.fTrackPosition;
	InstanceData.iRootBoneIndex = pAnimator->GetRootBoneIndex();
	
	if (pAnimator->IsBlending())
	{
		const auto& PrevAnimState = pAnimator->GetPrevAnimState();
		InstanceData.iPrevAnimIndex = static_cast<uint32_t>(PrevAnimState.iAnimIndex);
		InstanceData.fPrevTrackPosition = PrevAnimState.fTrackPosition;
		InstanceData.fBlendWeight = pAnimator->GetBlendWeight();
		InstanceData.bBlending = 1;
	}

	const auto eAnimatorMode = pAnimator->GetEvaluationMode();
	const uint32_t iEvaluationMode = static_cast<uint32_t>(eAnimatorMode == CComAnimator::EVALUATION_MODE::CPU? CComAnimator::EVALUATION_MODE::CPU_GPU: eAnimatorMode);
	
	MODEL_INSTANCE_BATCH* pBatch = Find_Or_Create_Batch(pModelInstance, false, iEvaluationMode);
	if (!pBatch) return;

	pBatch->ObjectHandle = pModelInstance->GetGameObject()->GetHandle();
	const uint32_t iBatchInstanceIndex = static_cast<uint32_t>(pBatch->Instances.size());
	if (CGameObject* pObject = CGameInstance::Get().GetGameObjectByHandle(pBatch->ObjectHandle))
		pObject->SetInstanceModelNum(iBatchInstanceIndex);

	pBatch->Instances.push_back(InstanceData);
	pBatch->CombinedBoneMatrices.push_back(pModelInstance->Get_CombinedBoneMatrices());

	/*----------- 광윤 추가 -----------*/
	std::optional<BoundingBox>	ShadowBounds;

	if (CGameObject* pObject = pModelInstance->GetGameObject()) {
		BoundingBox Bounds{};

		if (pObject->GetShadowBounds(Bounds))
			ShadowBounds = Bounds;
	}

	pBatch->ShadowBounds.push_back(ShadowBounds);
	/*---------------------------------*/

	++m_iTotalInstanceCount;
	if (!pBatch->bActiveThisFrame)
	{
		pBatch->bActiveThisFrame = true;
		m_ActiveBatches.push_back(pBatch);
	}
}


void CModel_Instance_Manager::Add_Instance(CComStaticModelInstance* pModelInstance,  const _float4x4& WorldMatrix, uint32_t iFlags)
{
	if (!pModelInstance)
	{
		return;
	}

	const auto& pModel = pModelInstance->GetModel();

	if (!pModel)
		return;


	GPU_ANIM_INSTANCE_DATA InstanceData{};

	InstanceData.WorldMatrix = WorldMatrix;

	InstanceData.iAnimIndex = INVALID_ANIM_INDEX;

	InstanceData.iFlags = iFlags;

	InstanceData.fTrackPosition = 0.f;

	InstanceData.iRootBoneIndex = 0;



	Add_Instance(pModelInstance, InstanceData);
}


void CModel_Instance_Manager::Add_Instance( CComModelInstance* pModelInstance, const GPU_ANIM_INSTANCE_DATA& InstanceData)
{
	if (!pModelInstance)
		return;

	const _bool bStaticModel = (InstanceData.iAnimIndex == INVALID_ANIM_INDEX);
	MODEL_INSTANCE_BATCH* pBatch = Find_Or_Create_Batch(pModelInstance, bStaticModel);

	if (!pBatch)
		return;


	
	pBatch->ObjectHandle = pModelInstance->GetGameObject()->GetHandle();
	const uint32_t iBatchInstanceIndex = static_cast<uint32_t>(pBatch->Instances.size());
	// Instance 踰덊?�瑜????�� ?꾪빐 ??�떆 Object???깅줉????�쨾????�떎.
	if (CGameObject* pObject = CGameInstance::Get().GetGameObjectByHandle(pBatch->ObjectHandle))
	{
		pObject->SetInstanceModelNum(iBatchInstanceIndex);
	}
	pBatch->Instances.push_back(InstanceData);
	++m_iTotalInstanceCount;



	if (!pBatch->bActiveThisFrame)
	{
		pBatch->bActiveThisFrame = true;
		m_ActiveBatches.push_back(pBatch);
	}
}

void CModel_Instance_Manager::Add_Part_Instance(CComStaticModelInstance* pModelInstance, const GPU_PART_INSTANCE_DATA& InstanceData)
{
	if (!pModelInstance || !pModelInstance->GetModel())
		return;

	MODEL_INSTANCE_BATCH* pBatch = Find_Or_Create_Part_Batch(pModelInstance);
	if (!pBatch)
		return;

	pBatch->ObjectHandle = pModelInstance->GetGameObject()->GetHandle();
	pBatch->PartInstances.push_back(InstanceData);
	if (!pBatch->bActiveThisFrame)
	{
		pBatch->bActiveThisFrame = true;
		m_ActiveBatches.push_back(pBatch);
	}
}
void CModel_Instance_Manager::Add_Instance(CComStaticModelInstance* pModelInstance, const GPU_ANIM_INSTANCE_DATA& InstanceData)
{
	if (!pModelInstance)
		return;

	const _bool bStaticModel = (InstanceData.iAnimIndex == INVALID_ANIM_INDEX);
	MODEL_INSTANCE_BATCH* pBatch = Find_Or_Create_Batch(pModelInstance, bStaticModel);

	if (!pBatch)
		return;



	pBatch->ObjectHandle = pModelInstance->GetGameObject()->GetHandle();
	const uint32_t iBatchInstanceIndex = static_cast<uint32_t>(pBatch->Instances.size());
	// Instance 踰덊?�瑜????�� ?꾪빐 ??�떆 Object???깅줉????�쨾????�떎.
	if (CGameObject* pObject = CGameInstance::Get().GetGameObjectByHandle(pBatch->ObjectHandle))
	{
		pObject->SetInstanceModelNum(iBatchInstanceIndex);
	}
	pBatch->Instances.push_back(InstanceData);
	++m_iTotalInstanceCount;



	if (!pBatch->bActiveThisFrame)
	{
		pBatch->bActiveThisFrame = true;
		m_ActiveBatches.push_back(pBatch);
	}
}



MODEL_INSTANCE_BATCH* CModel_Instance_Manager::Find_Or_Create_Batch(CComModelInstance* pModelInstance, _bool bStaticModel, uint32_t iEvaluationMode)
{
	if (!pModelInstance)
		return nullptr;

	const auto& pModel =pModelInstance->GetModel();

	if (!pModel)
		return nullptr;

	MODEL_INSTANCE_KEY Key{};

	Key.modelGroup = pModelInstance->Get_GroupTag();

	Key.modelTag = pModelInstance->Get_ResTag();
	Key.bStaticModel = bStaticModel;
	Key.iEvaluationMode = iEvaluationMode;

	auto Iter = m_InstanceBatches.find(Key);

	if (Iter != m_InstanceBatches.end())
	{
		return Iter->second.get();
	}

	auto pBatch = std::make_unique<MODEL_INSTANCE_BATCH>();

	if (!pBatch)
		return nullptr;

	pBatch->Key =Key;
	pBatch->bModelStatic = bStaticModel;
	pBatch->bGPUSkinned =
		iEvaluationMode == static_cast<uint32_t>(CComAnimator::EVALUATION_MODE::GPU);


	pBatch->Instances.reserve(16);
	pBatch->CombinedBoneMatrices.reserve(16);

	MODEL_INSTANCE_BATCH* pBatchRaw =pBatch.get();

	m_InstanceBatches.emplace(Key,std::move(pBatch));

	return pBatchRaw;
}

MODEL_INSTANCE_BATCH* CModel_Instance_Manager::Find_Or_Create_Part_Batch(CComStaticModelInstance* pModelInstance)
{
	MODEL_INSTANCE_KEY Key{};
	Key.modelGroup = pModelInstance->Get_GroupTag();
	Key.modelTag = pModelInstance->Get_ResTag();
	Key.bStaticModel = true;

	auto iter = m_InstanceBatches.find(Key);
	if (iter != m_InstanceBatches.end())
		return iter->second.get();

	auto pBatch = std::make_unique<MODEL_INSTANCE_BATCH>();
	pBatch->Key = Key;
	pBatch->bModelStatic = true;
	pBatch->PartInstances.reserve(16);
	auto* pBatchRaw = pBatch.get();
	m_InstanceBatches.emplace(Key, std::move(pBatch));
	return pBatchRaw;
}

MODEL_INSTANCE_BATCH* CModel_Instance_Manager::Find_Or_Create_Batch(CComStaticModelInstance* pModelInstance, _bool bStaticModel)
{
	if (!pModelInstance)
		return nullptr;

	const auto& pModel = pModelInstance->GetModel();

	if (!pModel)
		return nullptr;

	MODEL_INSTANCE_KEY Key{};

	Key.modelGroup = pModelInstance->Get_GroupTag();

	Key.modelTag = pModelInstance->Get_ResTag();
	Key.bStaticModel = bStaticModel;

	auto Iter = m_InstanceBatches.find(Key);

	if (Iter != m_InstanceBatches.end())
	{
		return Iter->second.get();
	}

	auto pBatch = std::make_unique<MODEL_INSTANCE_BATCH>();

	if (!pBatch)
		return nullptr;

	pBatch->Key = Key;
	pBatch->bModelStatic = bStaticModel;

	// ?몄뒪??�뒪 媛쒖???????�?�?
	pBatch->Instances.reserve(16);
	pBatch->CombinedBoneMatrices.reserve(16);
	/*----------- 광윤 추가 -----------*/
	pBatch->ShadowBounds.reserve(16);
	/*---------------------------------*/
	MODEL_INSTANCE_BATCH* pBatchRaw = pBatch.get();

	m_InstanceBatches.emplace(Key, std::move(pBatch));

	return pBatchRaw;
}
void CModel_Instance_Manager::Clear_Frame()
{
	for (MODEL_INSTANCE_BATCH* pBatch : m_ActiveBatches)
	{
		if (!pBatch)
			continue;

		pBatch->Instances.clear();
		pBatch->PartInstances.clear();
		pBatch->CombinedBoneMatrices.clear();
		/*----------- 광윤 추가 -----------*/
		pBatch->ShadowBounds.clear();
		/*---------------------------------*/
		pBatch->bActiveThisFrame = false;
	}

	m_ActiveBatches.clear();

	m_iTotalInstanceCount = 0;
}

void CModel_Instance_Manager::UpdateGUI()
{
	if (!ImGui::Begin("Model Instance Manager"))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Registered Batch Count : %zu",m_InstanceBatches.size());

	ImGui::Text("Active Batch Count : %zu",m_ActiveBatches.size());

	ImGui::Text("Total Instance Count : %u",m_iTotalInstanceCount);

	ImGui::Separator();

	uint32_t iBatchIndex = 0;

	for (const MODEL_INSTANCE_BATCH* pBatch :m_ActiveBatches)
	{
		if (!pBatch)
			continue;

		ImGui::PushID(
			static_cast<int32_t>(
				iBatchIndex));

		const uint32_t iInstanceCount =
			static_cast<uint32_t>(
				pBatch->Instances.size());

		char szTreeLabel[64]{};

		sprintf_s(
			szTreeLabel,
			"Batch %u",
			iBatchIndex);

		if (ImGui::TreeNode(szTreeLabel))
		{
			

			ImGui::Text(
				"Instance Count : %u",
				iInstanceCount);

			ImGui::Text(
				"Vector Capacity : %zu",
				pBatch->Instances.capacity());

			ImGui::Text(
				"Active : %s",
				pBatch->bActiveThisFrame
				? "True"
				: "False");

			ImGui::TreePop();
		}

		ImGui::PopID();

		++iBatchIndex;
	}

	ImGui::End();

}

HRESULT CModel_Instance_Manager::Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx)
{
	

	for (MODEL_INSTANCE_BATCH* pBatch : m_ActiveBatches)
	{
		if (!pBatch || (pBatch->Instances.empty() && pBatch->PartInstances.empty()))
			continue;

		auto* pObject = CGameInstance::Get().GetGameObjectByHandle(pBatch->ObjectHandle);
		if (pObject == nullptr)
			return E_FAIL;

		if (FAILED(pObject->Render_Instanced(pContext, ctx, *pBatch)))
			return E_FAIL;
	}

	return S_OK;
}

/*----------- 광윤 추가 -----------*/
HRESULT CModel_Instance_Manager::Render_ShadowInstanced(ID3D11DeviceContext* pContext, std::optional<CHandle> _LightHandle, _bool _bStaticBatch, int32_t _PointFaceIndex){
	
	if (!_LightHandle)	return E_FAIL;

	if (_PointFaceIndex < -1 || _PointFaceIndex >= static_cast<int32_t>(POINT_SHADOW_MAPCOUNT))	return E_FAIL;

	auto pLight = CGameInstance::Get().GetGameObjectByHandleT<CLight>(_LightHandle.value());
	if (nullptr == pLight) return E_FAIL;

	for (MODEL_INSTANCE_BATCH* pBatch : m_ActiveBatches) {
		if (!pBatch || pBatch->Instances.empty() || pBatch->bModelStatic != _bStaticBatch || pBatch->bGPUSkinned) continue;

		m_ShadowFilteredInstances.clear();
		m_ShadowVisibleSourceIndices.clear();

		const size_t InstanceCount = pBatch->Instances.size();
		const LIGHT_TYPE LightType = pLight->Get_LightType();

		for (size_t i = 0; i < InstanceCount; ++i) {
			_bool bVisibleToLight = true;
			if (LightType != LIGHT_TYPE::DIRECTIONAL && i < pBatch->ShadowBounds.size()) {
				const auto& OptionalBounds = pBatch->ShadowBounds[i];

				if (OptionalBounds.has_value()) {
					BoundingBox ShadowBound = OptionalBounds.value();

					constexpr _float ShadowCullPadding = 1.f;

					ShadowBound.Extents.x += ShadowCullPadding;
					ShadowBound.Extents.y += ShadowCullPadding;
					ShadowBound.Extents.z += ShadowCullPadding;

					bVisibleToLight = pLight->Intersects_ShadowBounds(ShadowBound);

					if (bVisibleToLight && LightType == LIGHT_TYPE::POINT && _PointFaceIndex >= 0) {
						bVisibleToLight = pLight->Intersects_PointShadowFace(static_cast<uint32_t>(_PointFaceIndex), ShadowBound);
					}
				}
			}

			if (false == bVisibleToLight)	continue;

			m_ShadowFilteredInstances.push_back(pBatch->Instances[i]);

			m_ShadowVisibleSourceIndices.push_back(static_cast<uint32_t>(i));
		}

		if (m_ShadowFilteredInstances.empty())	continue;

		if (FAILED(Render_ShadowBatch(pContext, *pBatch))) {
			ID3D11ShaderResourceView* NullSRVs[3]{};
			pContext->VSSetShaderResources(6, 3, NullSRVs);

			return E_FAIL;
		}
	}
	
	return S_OK;
}
HRESULT CModel_Instance_Manager::Render_ShadowBatch(ID3D11DeviceContext* pContext, const MODEL_INSTANCE_BATCH& Batch){
	const uint32_t iInstanceCount = static_cast<uint32_t>(m_ShadowFilteredInstances.size());
	if (iInstanceCount == 0 || iInstanceCount > MAX_INSTANCE_COUNT || m_ShadowVisibleSourceIndices.size() != iInstanceCount)	return E_FAIL;

	auto pModel = CGameInstance::Get().GetResourceFirst<CResModel>(Batch.Key.modelGroup, Batch.Key.modelTag);
	if (nullptr == pModel) return E_FAIL;

	const uint32_t BoneStride = static_cast<uint32_t>(pModel->GetBones().size());

	if (BoneStride == 0 || BoneStride > MAX_BONE_COUNT)	return E_FAIL;

	if (FAILED(Update_ShadowInstanceBuffer(pContext)))	return E_FAIL;

	if (FAILED(Update_BonePaletteBuffer(pContext, Batch, BoneStride)))		return E_FAIL;

	for (uint32_t iMeshIndex = 0; iMeshIndex < pModel->Get_NumMeshes(); ++iMeshIndex) {
		const auto& mesh = pModel->GetMeshes()[iMeshIndex];
		if (nullptr == mesh)	continue;

		if (FAILED(Bind_SkinMeshConstantBuffer(pContext, pModel, iMeshIndex, BoneStride)))	return E_FAIL;

		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const UINT stride = mesh->GetVertexStride();
		const UINT offset = 0;

		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		pContext->DrawIndexedInstanced(mesh->GetNumIndices(), iInstanceCount, 0, 0, 0);
	}

	ID3D11ShaderResourceView* pNullSRVs[3]{ };
	pContext->VSSetShaderResources(6, 3, pNullSRVs);

	return S_OK;
}

HRESULT CModel_Instance_Manager::Update_BonePaletteBuffer(ID3D11DeviceContext* pContext, const MODEL_INSTANCE_BATCH& Batch, uint32_t BoneStride) {
	const uint32_t InstanceCount = static_cast<uint32_t>(m_ShadowFilteredInstances.size());
	if (InstanceCount == 0 || InstanceCount > MAX_INSTANCE_COUNT || m_ShadowVisibleSourceIndices.size() != InstanceCount)	return E_FAIL;

	auto pBonePaletteBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_CPU_BONEMATRIX");
	if (!pBonePaletteBuffer)		return E_FAIL;

	const size_t RequiredMatrixCount = static_cast<size_t>(InstanceCount) * BoneStride;

	if (RequiredMatrixCount > m_ShadowBonePaletteScratch.size())	return E_FAIL;

	for (uint32_t FilteredIndex = 0; FilteredIndex < InstanceCount; ++FilteredIndex) {
		const uint32_t iSourceIndex = m_ShadowVisibleSourceIndices[FilteredIndex];
		if (iSourceIndex >= Batch.CombinedBoneMatrices.size())	return E_FAIL;

		const auto& CombinedMatrixList = Batch.CombinedBoneMatrices[iSourceIndex];

		if (CombinedMatrixList.size() != BoneStride) return E_FAIL;

		const size_t PaletteOffset = static_cast<size_t>(FilteredIndex) * BoneStride;

		for (uint32_t iBoneIndex = 0; iBoneIndex < CombinedMatrixList.size(); ++iBoneIndex) 
			XMStoreFloat4x4(&m_ShadowBonePaletteScratch[PaletteOffset + iBoneIndex], XMMatrixTranspose(XMLoadFloat4x4(&CombinedMatrixList[iBoneIndex])));
	}
	{
		ID3D11ShaderResourceView* NullSRV = nullptr;
		pContext->VSSetShaderResources(7, 1, &NullSRV);

		const uint32_t ByteSize = static_cast<uint32_t>(RequiredMatrixCount * sizeof(_float4x4));

		if (FAILED(pBonePaletteBuffer->UpdateData(m_ShadowBonePaletteScratch.data(), static_cast<uint32_t>(ByteSize))))		return E_FAIL;

		ComPtr<ID3D11ShaderResourceView> pBonePaletteSRV = pBonePaletteBuffer->GetSRV();
		if (nullptr == pBonePaletteSRV)	return E_FAIL;

		pContext->VSSetShaderResources(7, 1, pBonePaletteSRV.GetAddressOf());
	}
	
	return S_OK;
}

HRESULT CModel_Instance_Manager::Update_ShadowInstanceBuffer(ID3D11DeviceContext* pContext) {
	const size_t InstanceCount = m_ShadowFilteredInstances.size();
	if (InstanceCount == 0 || InstanceCount > MAX_INSTANCE_COUNT)	return E_FAIL;

	auto Buffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");
	if (nullptr == Buffer || nullptr == Buffer->GetBuffer())	return E_FAIL;
	
	const uint32_t ByteSize = sizeof(GPU_ANIM_INSTANCE_DATA) * static_cast<uint32_t>(InstanceCount);

	{
		ComPtr<ID3D11ShaderResourceView> NullSRV = nullptr;
		pContext->VSSetShaderResources(6, 1, NullSRV.GetAddressOf());

		if (FAILED(Buffer->UpdateData(m_ShadowFilteredInstances.data(), ByteSize)))	return E_FAIL;

		ComPtr<ID3D11ShaderResourceView> SRV = Buffer->GetSRV();
		if (nullptr == SRV)	return E_FAIL;

		pContext->VSSetShaderResources(6, 1, SRV.GetAddressOf());
	}
	
	return S_OK;
}

HRESULT CModel_Instance_Manager::Bind_SkinMeshConstantBuffer(ID3D11DeviceContext* pContext, SPtr<CResModel>& Model, uint32_t MeshIndex, uint32_t BoneStride) {
	if (MeshIndex >= Model->Get_NumMeshes())	return E_FAIL;

	const auto& mesh = Model->GetMeshes()[MeshIndex];
	if (nullptr == mesh) return E_FAIL;

	const auto& skinRange = Model->Get_GPUMeshSkinRange(MeshIndex);
	if (skinRange.iSkinBoneCount == 0)
		return E_FAIL;

	if (BoneStride == 0 || BoneStride > MAX_BONE_COUNT)	return E_FAIL;

	E::GPU_SKIN_MESH_CONSTANTS skinningConstants{};
	skinningConstants.iSkinBoneOffset = skinRange.iSkinBoneOffset;
	skinningConstants.iVertexCount = mesh->GetNumVertices();
	skinningConstants.iSkinBoneCount = skinRange.iSkinBoneCount;
	skinningConstants.iBonePaletteStride = BoneStride;
	auto SKM_CB = m_pResSkinMeshCBuffer->GetCBuffer().Get();
	if (!SKM_CB) return E_FAIL;

	D3D11_MAPPED_SUBRESOURCE MRES{};
	if (SUCCEEDED(pContext->Map(SKM_CB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MRES)))
	{
		memcpy(MRES.pData, &skinningConstants, sizeof(skinningConstants));
		pContext->Unmap(SKM_CB, 0);
	}
	else { return E_FAIL; }
	pContext->VSSetConstantBuffers(5, 1, &SKM_CB);

	ID3D11ShaderResourceView* pSkinBonesSRV = Model->Get_GPUSkinBoneSRV();

	if (!pSkinBonesSRV)	return E_FAIL;

	pContext->VSSetShaderResources(8, 1, &pSkinBonesSRV);

	return S_OK;
}
_bool CModel_Instance_Manager::Has_ActiveDynamicShadowBatch() {
	for (const MODEL_INSTANCE_BATCH* Batch : m_ActiveBatches)
	{
		if (nullptr == Batch || Batch->Instances.empty() || Batch->bModelStatic || Batch->bGPUSkinned)
			continue;

		return true;
	}

	return false;
}
/*---------------------------------*/

UPtr<CModel_Instance_Manager> CModel_Instance_Manager::Create()
{
	auto pInstance =ToUPtr(new CModel_Instance_Manager{});

	if (FAILED( pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CModel_Instance_Manager");
		return nullptr;
	}

	return pInstance;
}
