#include "pch.h"
#include "Monster.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "ComModelInstance.h"
#include "ComAnimator.h"
#include "Resources.h"
#include "ComBeHavior.h"
#include "Mon_Weapon.h"
#include "GameInstance.h"
#include "ComCollider.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "Player_Magic_Bullet.h"

#include "CollBox.h"
#include "UIManager.h"
#include "UIController.h"

#include "ComPxRigidBody.h"
#include "ComPxSphereCollider.h"
#include "ClientEvents.h"

#include "UIManager.h"
#include "ComSound.h"
NS_USING(Client)

CMonster::CMonster()
{
}


CMonster::~CMonster()
{
	// 구독해제
	// CGameInstance::Get().EventUnsubscribeAll(GetHandle());
}

void CMonster::UpdateGUI()
{
	CGameObject::UpdateGUI();
	ImGui::DragInt("HP", &m_iHp, 0, 1);
	ImGui::DragFloat("EE", &m_fIntensive, 0.1f,0.f,100.f);
	ImGui::DragFloat3("ff", reinterpret_cast<_float*>(&m_fEMissiveColor), 0.1f,0.f, 1.f);
	ImGui::Text("NoramlAtt : %d", m_iNormalHitCnt);
	
	ImGui::Text("bPending : %s", m_bPending == true ? "TRUE" : "FALSE");
	ImGui::Text("Pending AttType : %s", MagicEnumToStringView(m_PendingMonTable.eAttType).data());
	ImGui::Text("Pending HitType : %s", MagicEnumToStringView(m_PendingMonTable.eHitType).data());

	ImGui::Separator();
	ImGui::Text("ActiveHit : %s", m_bActiveHit == true ? "TRUE" : "FALSE");
	ImGui::Text("ActiveHit AttType : %s", MagicEnumToStringView(m_ActiveMonTable.eAttType).data());
	ImGui::Text("ActiveHit HitType : %s", MagicEnumToStringView(m_ActiveMonTable.eHitType).data());
	ImGui::Separator();
	ImGui::Text("Current Attack:"); ImGui::SameLine();
	ImGui::Text(MagicEnumToStringView(m_eAttType).data());

	if (nullptr != m_pBeHavior)
		ImGui::Text("BeHavior Att : %s", Check_Flag(ETOUI(CBTRoot::BTFLAG::ATTACK)) == true ? "ENABLE" : "DISABLE");

	ImGui::Text(m_CurEffectName.c_str());
	if (ImGui::TreeNode("Flag"))
	{
		struct GuiView
		{
			uint32_t iValue{};
			const _char* pName{};
		};
#define X(name, value) value, #name,
		const GuiView Flags[] = { BTFLAG_M };
#undef X

		for (uint32_t i = 0; i < std::size(Flags); ++i)
		{
			ImGui::PushID(i);
			ImGui::Text(Flags[i].pName); ImGui::SameLine();
			ImGui::Text(true == m_pBeHavior->Check_Flag(Flags[i].iValue) ? ": TRUE" : " FALSE");
			ImGui::SameLine();
			if (ImGui::Button("Invert"))
			{
				m_pBeHavior->Set_Flag(Flags[i].iValue, FLAGTYPE::INVERT);
			}
			ImGui::PopID();
		}

		ImGui::TreePop();
	}
}

HRESULT CMonster::InitializePrototype(void* pArg)
{
	m_pResVertexCPUSkinningInstancedShader = CGameInstance::Get().GetResourceFirst<CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim_CPU_Skinning_Instanced");
	if (!m_pResVertexCPUSkinningInstancedShader || FAILED(m_pResVertexCPUSkinningInstancedShader->Load()))
	{
		return E_FAIL;
	}
	m_pResPixelShader = CGameInstance::Get().GetResourceFirst<CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim");
	if (FAILED(m_pResPixelShader->Load()))
	{
		return E_FAIL;
	}
	m_pResSkinMeshCBuffer = CGameInstance::Get().GetResourceFirst<CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_GPU_SKIN_MESH");
	if (!m_pResSkinMeshCBuffer)
	{
		return E_FAIL;
	}


	return S_OK;
}

HRESULT CMonster::Initialize(void* pArg)
{
	auto MonDesc = static_cast<MONSTER_DESC*>(pArg);
	m_bDonMove = MonDesc->bDonMove;
	m_TargetHandle = MonDesc->TargetHandle;
	if (FAILED(CGameObject::Initialize(pArg)))
	{
		return E_FAIL;
	}
	{
		CComSound::DESC Desc{};
		if (FAILED(AddComponentFromProto(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoComponent::Prototype_Component_ComSound,
			"Com_Sound",
			&Desc,
			&m_pComSound)))
		{
			return E_FAIL;
		}

		CGameInstance::Get().EventSubscribe<FAcientMagicStart>(GetHandle(), [=]() { Cancle_Attack(); });
	}
	return S_OK;
}


void CMonster::PriorityUpdate(E::_float fTimeDelta)
{
	Activate_PendingHit();
	if (CGameInstance::Get().KeyPressing(DIK_LCONTROL) && CGameInstance::Get().KeyDown(DIK_0))
		m_pMoveIntent->RequestWarp(_float3(20, 20, 20));
	
	m_pMoveIntent->ClearMoveIntent();
	m_pMoveIntent->ClearFacingIntent();
	__super::PriorityUpdate(fTimeDelta);
	
	if (m_pBeHavior->Check_Flag(ETOUI(CBTRoot::BTFLAG::DROP)| ETOUI(CBTRoot::BTFLAG::DEAD) | ETOUI(CBTRoot::BTFLAG::DEBRIS)))
		m_pCharacterMotor->SetUseGravity(true);
	else m_pCharacterMotor->SetUseGravity(false);
		
	Flag_Check(fTimeDelta);
	m_pCharacterMotor->SetGravity(-9.8f);
	m_pBeHavior->Update(fTimeDelta);
	
}

void CMonster::Update(E::_float fTimeDelta)
{
	__super::Update(fTimeDelta);
	if (!m_pComSound)
		m_pComSound->Update();
	if (m_pComModelInstance->GetModel()->GetAnimations().size() != 0)
		m_pModelAnimator->Update(fTimeDelta);

	EmissiveFadeOut(fTimeDelta);
	m_pBeHavior->AbortNode();
	Update_HurtBox();
}

void CMonster::LateUpdate(E::_float fTimeDelta)
{
	__super::LateUpdate(fTimeDelta);
	const _float3 vControllerPosition = m_pCharacterController->GetPosition();
	GetTransform().SetPosition(m_pCharacterController->GetFootPosition());
	GetTransform().Update();
	const auto& pModel = m_pComModelInstance->GetModel();

	if (!pModel)
		return;

	if (!pModel->GetAnimations().empty())
	{
		CGameInstance::Get().Add_Instance(m_pComModelInstance, m_pModelAnimator, *GetTransform().GetCombinedWorldMatrix());

		return;
	}
}
HRESULT CMonster::Render_Instanced(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx, const E::MODEL_INSTANCE_BATCH& Batch)
{
	if (!pContext || !m_pResVertexCPUSkinningInstancedShader || !m_pResPixelShader)
		return E_FAIL;

	const auto& vs = m_pResVertexCPUSkinningInstancedShader;
	const auto& ps = m_pResPixelShader;
	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	const uint32_t iInstanceCount = static_cast<uint32_t>(Batch.Instances.size());
	if (iInstanceCount == 0 || iInstanceCount > 512 || Batch.CombinedBoneMatrices.size() != iInstanceCount)
		return E_FAIL;

	if (FAILED(Update_InstanceBuffer(pContext, Batch.Instances)))
		return E_FAIL;

	auto pModel = CGameInstance::Get().GetResourceFirst<CResModel>(Batch.Key.modelGroup, Batch.Key.modelTag);
	auto pCPUBonePaletteBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_CPU_BONEMATRIX");
	if (!pModel || !pCPUBonePaletteBuffer)
		return E_FAIL;

	_float4x4 identity{};
	XMStoreFloat4x4(&identity, XMMatrixIdentity());
	std::vector<_float4x4> combinedPalette(iInstanceCount * 512, identity);
	for (uint32_t instanceIndex = 0; instanceIndex < iInstanceCount; ++instanceIndex)
	{
		const auto& combinedMatrices = Batch.CombinedBoneMatrices[instanceIndex];
		if (combinedMatrices.empty() || combinedMatrices.size() > 512)
			return E_FAIL;

		// DirectXMath로 계산한 CPU Combined 행렬을 VS의 t7 행렬 규약에 맞춘다.
		// CPU 원본은 다른 CPU 기능에서도 사용하므로 업로드 복사본만 전치한다.
		for (uint32_t boneIndex = 0; boneIndex < static_cast<uint32_t>(combinedMatrices.size()); ++boneIndex)
		{
			XMStoreFloat4x4(
				&combinedPalette[instanceIndex * 512 + boneIndex],
				XMMatrixTranspose(
					XMLoadFloat4x4(&combinedMatrices[boneIndex])));
		}
	}

	// CPU가 계산한 CombinedBone palette는 batch당 한 번만 갱신한다.
	ID3D11ShaderResourceView* nullPaletteSRV = nullptr;
	pContext->VSSetShaderResources(7, 1, &nullPaletteSRV);
	if (FAILED(pCPUBonePaletteBuffer->UpdateData(
		combinedPalette.data(),
		static_cast<uint32_t>(combinedPalette.size() * sizeof(_float4x4)))))
		return E_FAIL;



	if (FAILED(Bind_InstanceBuffer(pContext)))
		return E_FAIL;
	ID3D11ShaderResourceView* cpuBonePaletteSRV = pCPUBonePaletteBuffer->GetSRV().Get();
	if (!cpuBonePaletteSRV)
		return E_FAIL;

	ID3D11ShaderResourceView* skinBonesSRV = pModel->Get_GPUSkinBoneSRV();
	if (!skinBonesSRV)
		return E_FAIL;

	pContext->VSSetShaderResources(7, 1, &cpuBonePaletteSRV);
	pContext->VSSetShaderResources(8, 1, &skinBonesSRV);

	for (uint32_t iMeshIndex = 0; iMeshIndex < pModel->Get_NumMeshes(); ++iMeshIndex)
	{
		const auto& mesh = pModel->GetMeshes()[iMeshIndex];
		if (!mesh)
			continue;

		const auto& skinRange = pModel->Get_GPUMeshSkinRange(iMeshIndex);
		if (skinRange.iSkinBoneCount == 0)
			return E_FAIL;

		E::GPU_SKIN_MESH_CONSTANTS skinningConstants{};
		skinningConstants.iSkinBoneOffset = skinRange.iSkinBoneOffset;
		skinningConstants.iVertexCount = mesh->GetNumVertices();
		skinningConstants.iSkinBoneCount = skinRange.iSkinBoneCount;
		D3D11_MAPPED_SUBRESOURCE mapped{};
		if (FAILED(pContext->Map(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
			return E_FAIL;
		memcpy(mapped.pData, &skinningConstants, sizeof(skinningConstants));
		pContext->Unmap(m_pResSkinMeshCBuffer->GetCBuffer().Get(), 0);
		ID3D11Buffer* skinningCB = m_pResSkinMeshCBuffer->GetCBuffer().Get();
		pContext->VSSetConstantBuffers(5, 1, &skinningCB);
		ID3D11Buffer* vertexBuffer = mesh->GetVertexBuffer().Get();
		const UINT stride = mesh->GetVertexStride();
		const UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		pContext->IASetIndexBuffer(mesh->GetIndexBuffer().Get(), mesh->GetIndexFormat(), 0);
		pContext->IASetPrimitiveTopology(mesh->GetPrimitiveType());
		m_pComModelInstance->Bind_Textures(pContext, iMeshIndex);
		m_pComModelInstance->Bind_Materials(pContext, m_fEMissiveColor, m_fIntensive, { 1.f, 1.f, 1.f }, 0.f, 1.f);
		pContext->DrawIndexedInstanced(mesh->GetNumIndices(), iInstanceCount, 0, 0, 0);
	}

	ID3D11ShaderResourceView* nullVSSRVs[3]{};
	pContext->VSSetShaderResources(6, 3, nullVSSRVs);

	return S_OK;

}
HRESULT CMonster::Update_InstanceBuffer(ID3D11DeviceContext* pContext, const std::vector<GPU_ANIM_INSTANCE_DATA>& Instances)
{

	m_iCurrentInstanceCount = static_cast<uint32_t>(Instances.size());

	if (Instances.empty())
		return S_OK;

	constexpr uint32_t MAX_INSTANCE_COUNT = 512;

	if (m_iCurrentInstanceCount > MAX_INSTANCE_COUNT)
		return E_FAIL;

	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11Buffer* pBuffer = pStructuredBuffer->GetBuffer().Get();

	if (!pBuffer)
		return E_FAIL;

	/*
	 * 이전 Batch에서 VS/CS에 연결되어 있을 수 있으므로
	 * Map 전에 SRV를 해제한다.
	 */
	ID3D11ShaderResourceView* pNullSRV = nullptr;

	pContext->CSSetShaderResources(6, 1, &pNullSRV);

	pContext->VSSetShaderResources(6, 1, &pNullSRV);

	const size_t iCopySize = sizeof(GPU_ANIM_INSTANCE_DATA) * m_iCurrentInstanceCount;
	D3D11_BOX updateBox{};
	updateBox.left = 0;
	updateBox.right = static_cast<UINT>(iCopySize);
	updateBox.top = 0;
	updateBox.bottom = 1;
	updateBox.front = 0;
	updateBox.back = 1;

	pContext->UpdateSubresource(pBuffer, 0, &updateBox, Instances.data(), 0, 0);

	return S_OK;

}
HRESULT CMonster::Bind_InstanceBuffer(ID3D11DeviceContext* pContext)
{
	auto pStructuredBuffer = CGameInstance::Get().GetResourceFirst<CResStructuredBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "SBUFFER_ANIMAITON");

	if (!pStructuredBuffer)
		return E_FAIL;

	ID3D11ShaderResourceView* pSRV = pStructuredBuffer->GetSRV().Get();

	if (!pSRV)
		return E_FAIL;

	// VS의 t6 슬롯에 InstanceData 연결
	pContext->VSSetShaderResources(6, 1, &pSRV);

	return S_OK;
}

/*----------- 광윤 추가 -----------*/
HRESULT CMonster::Render_Shadow(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx){
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
bool CMonster::GetShadowBounds(BoundingBox& OutBounds) const
{
	if (!m_pComCollider || !m_pComCollider->Get())	return false;

	CCollider* pCollider = m_pComCollider->Get();

	if (pCollider->GetCollType() != CollType::Box)	return false;

	const auto* pBox = static_cast<const CCollBox*>(pCollider);

	pBox->GetLocalBoundingBox().Transform(OutBounds, GetTransform().GetLoadedCombinedWorldMatrix());

	OutBounds.Extents.x *= 1.25f;
	OutBounds.Extents.y *= 1.25f;
	OutBounds.Extents.z *= 1.25f;

	return true;
}
/*---------------------------------*/

void CMonster::OnTriggerEnter(CGameObject* pObj, const PX_ON_TRIGGER_DATA& info)
{
	if (nullptr == pObj)
		return;

	
	if (auto pPlayerMagicBullet = Cast<CPlayer_Magic_Bullet>(pObj))
	{
		Check_Table(PLAYER_SKILL_TYPE::ATTACK);
	}
	
}
_bool CMonster::Activate_PendingHit()
{
	if (!m_bPending)return false;

	_bool bSameHit = m_bActiveHit &&
		m_ActiveMonTable.eAttType == m_PendingMonTable.eAttType &&
		m_ActiveMonTable.eHitType == m_PendingMonTable.eHitType;

	if (!bSameHit)
		++m_iHitCnt;

	m_ActiveMonTable = m_PendingMonTable;
	m_bActiveHit = true;

	m_PendingMonTable = {};
	m_bPending = false;

	return true;
}

void CMonster::ReActiveTable()
{
	m_PendingMonTable = {};
	m_bPending = false;

	m_ActiveMonTable = {}; 
	m_bActiveHit = false;
	m_iHitCnt = 0;
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::HIT), FLAGTYPE::DEL);
}

_bool CMonster::Is_Grounded()
{
	return m_pCharacterController->IsGrounded();
}

uint32_t CMonster::Find_SkillNum(ATTMON eType)
{
	auto iter = m_MonSkillLists.find(eType);
	
	if (iter == m_MonSkillLists.end())
		return UINT_MAX;
	return iter->second;

}

_bool CMonster::Check_Flag(uint32_t iFlag)
{
	return m_pBeHavior->Check_Flag(iFlag);
}

SOUND_ID  CMonster::Play_Sound(const MONSOUND& MonSound)
{
	auto iter = m_SoundTable.find(MonSound.SoundKey);

	if (iter == m_SoundTable.end() || iter->second.empty())
		return  INVALID_SOUND_ID;

	auto& SoundPaths = iter->second;

	int32_t iSoundIndex = Engine::RandInt(0, static_cast<int32_t>(SoundPaths.size()) - 1);

	SOUND_3D_DESC Sounds = MonSound.str3DSound;
	Sounds.vPosition = GetTransform().GetPosition();

	auto id = CGameInstance::Get().GetSoundManager()->Play3D(
		SoundPaths[iSoundIndex],
		Sounds,
		MonSound.SoundPlay
	);
	if (id == INVALID_SOUND_ID)
	{
		MSG_BOX("INVALID_SOUND_ID");
	}
	return id;
}

void CMonster::Skill_Finished()
{
	m_eAttType = ATTMON::END;
	m_CurEffectName.clear();
	m_eLastSkillTable = ATTMON::END;
	m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::EFFECT) | ETOUI(CBTRoot::BTFLAG::ATTACK) | ETOUI(CBTRoot::BTFLAG::ENDHIT) |ETOUI(CBTRoot::BTFLAG::THROW),FLAGTYPE::DEL);
}

void CMonster::Get_SoundKey(_string& CurSoundName)
{
	_string Key = "";
	if (ImGui::BeginCombo("SoundTable",CurSoundName.c_str()))
	{
		for (auto&[key, value] : m_SoundTable)
		{
			_bool bSelect = key == CurSoundName;
			if (ImGui::Selectable(key.c_str(), bSelect))
			{
				CurSoundName = key;
				break;
			}

			if(bSelect)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}
	return;
}

const _float4x4* CMonster::Get_CombineBoneMatrix(int32_t iBoneIndex)
{
	if (iBoneIndex >= m_pComModelInstance->Get_CombinedBoneMatrices().size() || iBoneIndex < 0)
		return nullptr;

	return &m_pComModelInstance->Get_CombinedBoneMatrices()[iBoneIndex];
}

CComAnimator* CMonster::Get_Animator()
{
	return m_pModelAnimator;
}

CComCharacterMoveIntent* CMonster::Get_MoveIntent()
{
	return m_pMoveIntent;
}

CBTBlackBoard* CMonster::Get_BlackBoard()
{
	if (nullptr == m_pBeHavior) return nullptr;
	return m_pBeHavior->Get_Blackboard();
}
void CMonster::Damaged(PLAYER_SKILL_TYPE eType)
{
	switch (eType)
	{
	case PLAYER_SKILL_TYPE::ATTACK:
		GET_SINGLE(UIManager)->CreateDamageFont(5, GetHandle(), false);
		m_iHp -= 5.f;
		break;
	case PLAYER_SKILL_TYPE::ACCIO:
		GET_SINGLE(UIManager)->CreateDamageFont(10, GetHandle(), true);
		m_iHp -= 10.f;
		break;
	case PLAYER_SKILL_TYPE::DEPULSO:
		GET_SINGLE(UIManager)->CreateDamageFont(15, GetHandle(), true);
		m_iHp -= 15.f;
		break;
	case PLAYER_SKILL_TYPE::DESCENDO:
		GET_SINGLE(UIManager)->CreateDamageFont(20, GetHandle(), true);
		m_iHp -= 20.f;
		break;
	case PLAYER_SKILL_TYPE::ACIENT_LIGHTNING:
		GET_SINGLE(UIManager)->CreateDamageFont(25, GetHandle(), true);
		m_iHp -= 25.f;
		break;
	case PLAYER_SKILL_TYPE::PROTEGO:
		m_iHp -= 8.f;
		break;
	}
}

void CMonster::Update_HurtBox()
{
	_bool bHurtBoxUpdated{ false };

	if (m_iColliderBoneIndex >= 0 && m_pComModelInstance)
	{
		const auto& CombinedBones =m_pComModelInstance->Get_CombinedBoneMatrices();

		const size_t iBoneIndex =
			static_cast<size_t>(m_iColliderBoneIndex);

		if (iBoneIndex < CombinedBones.size())
		{
			const _matrix HurtBoxWorld =
				XMLoadFloat4x4(&CombinedBones[iBoneIndex]) *
				GetTransform().GetLoadedCombinedWorldMatrix();

			_vector vScale{};
			_vector vRotation{};
			_vector vTranslation{};

			if (XMMatrixDecompose(
				&vScale,
				&vRotation,
				&vTranslation,
				HurtBoxWorld))
			{
				_float4 vHurtBoxRotation{};

				// 계산한 위치를 멤버에 저장
				XMStoreFloat3(
					&m_vHurtBoxPosition,
					vTranslation);

				XMStoreFloat4(
					&vHurtBoxRotation,
					XMQuaternionNormalize(vRotation));

				bHurtBoxUpdated = m_pComRigidBody->SetKinematicTarget(m_vHurtBoxPosition,vHurtBoxRotation);
			}
		}
	}

	if (!bHurtBoxUpdated)
	{
		m_vHurtBoxPosition =
			m_pCharacterController->GetPosition();

		m_pComRigidBody->SetKinematicTarget(
			m_vHurtBoxPosition,
			GetTransform().GetQuaternion());
	}
}

void CMonster::Cancle_Attack()
{
	if (!Check_Flag(ETOUI(CBTRoot::BTFLAG::GROGY)) && !Check_Flag(ETOUI(CBTRoot::BTFLAG::NOCKDOWN)) && Is_Grounded())
	{
		if (m_iEventBoneIndex == -1)
			return;
		ReActiveTable();
		m_pModelAnimator->Play_Anim(m_iEventBoneIndex, false, 0.1f);
		//여기에 블랙보드로 잠금 하기
		// 
		//m_pComModelInstance->Play_Anim(m_Value.iAnimIndex, m_bLoop, m_fBlend);

	}
}

void CMonster::Flag_Check(_float fTimeDelta)
{
	//이미시브
	if (m_fIntensive <= 0.f && Check_Flag(ETOUI(CBTRoot::BTFLAG::EMISSIVE)))
	{
		StartEmissive();
		m_bWork = true;
	}

	//뭔말알?
	if (m_iHp <= 0.f)
		m_pBeHavior->Set_Flag(ETOUI(CBTRoot::BTFLAG::DEAD), FLAGTYPE::ADD);

	if (Check_Flag(ETOUI(CBTRoot::BTFLAG::HIT)))
		m_fIntensive = 0;

	if (Check_Flag(ETOUI(CBTRoot::BTFLAG::ENDHIT)))
		Skill_Finished();

	if (!Check_Flag(ETOUI(CBTRoot::BTFLAG::LOOP)) && m_bSkillLoop)
	{
		if (m_iCurEffectID != INVALID_EFFECT_INSTANCE_ID)
			CGameInstance::Get().StopEffect(m_iCurEffectID);
		m_bSkillLoop = false;
	}
	
}
void CMonster::EmissiveFadeOut(_float fTimeDelta)
{
	if (m_fIntensive > 0.f &&  !Check_Flag(ETOUI(CBTRoot::BTFLAG::EMISSIVE)))
	{
		m_bWork = true;
		m_fTimeTick += fTimeDelta;

		_float t = m_fTimeTick / 0.5f;

		m_fIntensive = std::lerp(m_fPreEmissive, 0, t);
		if (t >= 1.f)
		{
			m_bWork = false;
			m_fTimeTick = m_fIntensive = 0;
		}

	}

}




