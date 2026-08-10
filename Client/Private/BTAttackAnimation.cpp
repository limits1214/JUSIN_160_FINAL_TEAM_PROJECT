#include "pch.h"
#include "BTAttackAnimation.h"
#include "ComAnimator.h" 
#include "ComCharacterMoveIntent.h"
#include "Monster.h"
#include "Player.h"
#include "ClientEvents.h" 
NS_USING(Client)

CBTAttackAnimation::CBTAttackAnimation()
{

}
CBTAttackAnimation::CBTAttackAnimation(const CBTAttackAnimation& rhs) : CBTAnimRoot(rhs)
{

}

CBTAttackAnimation::~CBTAttackAnimation()
{

}
HRESULT CBTAttackAnimation::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_MasterName = "BTAttackAnimation";
	return S_OK;
}
HRESULT CBTAttackAnimation::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTAttackAnimation::Evaluate(_float fTimeDelta)
{
	if (auto pBT = Get_ComBT())
	{
		if (auto pOwner = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			if (auto pTarget = pOwner->Get_Target())
			{
				_vector vDestPos = pTarget->GetTransform().GetState(STATE::POSITION);
				auto pAnimator = (Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));
				auto pTransform = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));
				auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");

				if (pTransform == nullptr || pAnimator == nullptr || pMoveIntent == nullptr ||
					-1 == m_Value.iAnimIndex)
					return m_eDebug = EVALUATE::FAILED;
				_vector vSrcPos = pTransform->GetState(STATE::POSITION);
				pAnimator->SetPlay(true);
				pAnimator->Play_Anim(m_Value.iAnimIndex, m_bLoop, m_fBlend);
				if (m_bStart)
				{
					m_fDis = XMVectorGetX(XMVector3Length(vDestPos - vSrcPos));
					m_bStart = false;
				}

				_float fAnimRatio = pAnimator->GetPlayAnimRatio();
				if (!m_bTrigger && !m_bActiveSkill)
					m_bActiveSkill = Active_Skill();
				
				if (m_bTrigger)
				{
					for (auto& iter :  m_Skills)
					{
						if (true == iter.bTrigger || iter.eSkill == ATTMON::END)
							continue;
						if (fAnimRatio >= iter.fRatio)
						{
							iter.bTrigger = true;
							ActiveTriggerSkill(iter.eSkill);
						}
							
					}
				}
				
				Gravity();
				Play_Sound(fTimeDelta);
				_bool bFinished = pAnimator->GetFinish();
				//살려주세요 살려주세요!!!
				Att(pOwner, pTransform, pTarget, fAnimRatio,fTimeDelta);
				EventFlagToRatio(fAnimRatio);
				ShakeCam(fAnimRatio);
				Rotation(pTransform, pMoveIntent, pTarget, fTimeDelta, fAnimRatio);
				//애니매이션 진행시간에 맞춰서 이동량 제어하기 m_bRatio true일 경우에만
			
				if (m_bRatio && m_fRatio.x <= fAnimRatio && m_fRatio.y >= fAnimRatio)
				{
					m_fTime += fTimeDelta;

					_float tt = (fAnimRatio - m_fRatio.x) / (m_fRatio.y - m_fRatio.x);
					if (tt < 0.f)
						tt = 0.f;
					if (tt > 1.f)
						tt = 1.f;
					
					if (auto pBT = Get_ComBT())
					{
						if (pBT->Check_Flag(ETOUI(CBTRoot::BTFLAG::EMISSIVE)))
						{
							if (auto pSrc = pBT->GetGameObject())
							{
								_float fEmissive = std::lerp(0.f, m_fIntensive, tt);
								static_cast<CMonster*>(pSrc)->Set_Emissive(fEmissive);
							}
						}
						
					}
					_float fAnimRange = m_fRatio.y - m_fRatio.x;
					_float t = (m_fDis * fAnimRatio) / (m_fRatio.y - m_fRatio.x);
					const _float fMoveSpeed = t * fAnimRange * m_Value.fSpeed;
					_vector vMoveDirection{};
					if (m_eMove == MOVE::RIGHT)
						vMoveDirection = pTransform->GetState(STATE::RIGHT);
					else if (m_eMove == MOVE::LEFT)
						vMoveDirection = -pTransform->GetState(STATE::RIGHT);
					else if (m_eMove == MOVE::STRAIGHT)
						vMoveDirection = pTransform->GetState(STATE::LOOK);
					else if (m_eMove == MOVE::BACKWARD)
						vMoveDirection = -pTransform->GetState(STATE::LOOK);
					else if (m_eMove == MOVE::UP)
						vMoveDirection = XMVectorSet(0, 1, 0, 0);

					if (m_eMove != MOVE::END)
					{
						_float3 vDirection{};
						XMStoreFloat3(&vDirection, vMoveDirection);
						pMoveIntent->SetMoveIntent(vDirection, fMoveSpeed);
					}
				}
				if (m_bEarly && m_fEarlyRatio <= fAnimRatio)
					return m_eDebug = EVALUATE::SUCCESS;

				if (m_bLoop || bFinished)
					return m_eDebug = EVALUATE::SUCCESS;
			
			}
		}
	}

	return m_eDebug = EVALUATE::RUN;
}
void CBTAttackAnimation::Update_Gui()
{
	__super::Update_Gui();
	if (ImGui::TreeNode("Attanim"))
	{
		DragFloat("Move Speed", m_Value.fSpeed);
		DragFloat("Intensive", m_fIntensive);
		DragFloat("ShakeCamRatio", m_fCamShakeRatio);
		ImGui::Text("OverLabRatio");
		ImGui::DragFloat2("##OverLabRatio", reinterpret_cast<_float*>(&m_vOverlabRatio), 0.1f, 0.f, 1.f);
		DragFloat("AttRadius", m_fAttRadius);
		BoolButton("TriggerSkill", m_bTrigger);
		BoolButton("overlabLoop", m_bOverLabLoop);
		BoolButton("overlabMove", m_bOverLabMove);
		DragFloat("overlabSpeed", m_fOverLabSpeed);
		DragFloat("RotTime", m_Value.fTime);
		if (ImGui::Button("Animation"))
			m_bPopup = true;
		if (m_bPopup)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
			if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::RB))
				m_bPopup = false;
			int32_t iIndex = CGameInstance::Get().GetAnimIndex(m_Handle);

			if (-1 != iIndex)
			{
				m_bPopup = false;
				m_Value.iAnimIndex = iIndex;
			}
			ImGui::PopStyleColor();
		}
#define X(name)#name,
		const _char* pMoveType[] = { MOVE_M "NONE" };
#undef X
		ImGui::Text("Move Selector");
		if (ImGui::BeginCombo("##Move Seletor", pMoveType[(ETOUI(m_eMove))]))
		{
			for (uint32_t i = 0; i < ETOUI(MOVE::END) + 1; ++i)
			{
				_bool bSelect = static_cast<int32_t>(m_eMove) == i;

				if (ImGui::Selectable(pMoveType[i]))
					m_eMove = static_cast<MOVE>(i);

				if (bSelect)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}
		ImGui::TreePop();
	}

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1,0,0,1 });
	if (ImGui::TreeNode("New Skill Table Ver.2"))
	{
		if (ImGui::Button("Add"))
		{
			m_Skills.push_back(ATT_SKILL_EVENT{});
		}

		if (!m_Skills.empty())
		{
			auto pBT = Get_ComBT();
			if (nullptr == pBT)
			{
				ImGui::TreePop(); ImGui::PopStyleColor();
				return;
			}
			auto pSrc = static_cast<CMonster*>(pBT->GetGameObject());
			if (nullptr == pSrc)
			{
				ImGui::TreePop(); ImGui::PopStyleColor();
				return;
			}
			uint32_t iButton{};
			for (auto iter = m_Skills.begin(); iter != m_Skills.end(); ++ iter)
			{
				ImGui::PushID(iButton);
				DragFloat("StartRatio",(*iter).fRatio);
				ImGui::SameLine();
				ImGui::Text("AttMon Type:"); ImGui::SameLine();
				if (ImGui::BeginCombo("##AttMon Type", pSrc->Get_SkillName((*iter).eSkill).c_str()))
				{
					for (uint32_t i = 0; i < ETOUI(ATTMON::END) + 1; ++i)
					{
						_string SkillName = pSrc->Get_SkillName(static_cast<ATTMON>(i));
						if (SkillName == "")
							continue;

						_bool bSelect = static_cast<int32_t>((*iter).eSkill) == i;

						if (ImGui::Selectable(SkillName.data()))
							(*iter).eSkill = static_cast<ATTMON>(i);

						if (bSelect)
							ImGui::SetItemDefaultFocus();

					}
					ImGui::EndCombo();
				}

				ImGui::SameLine();
				if (ImGui::Button("Del"))
				{
					m_Skills.erase(iter);
					ImGui::PopID();
					break;
				}
				ImGui::PopID();
				++iButton;
			}

		}
		

		ImGui::TreePop();
	}
	ImGui::PopStyleColor();
}
void CBTAttackAnimation::Att(CMonster* pMon, CComTransform* pSrcTransform, CGameObject* pTarget, _float fRotRatio,_float fTimeDelta)
{
	if (m_bAttRatio || (m_vOverlabRatio.x == 0.f && m_vOverlabRatio.y == 0.f))
		return;

	if (m_vOverlabRatio.y >= fRotRatio && fRotRatio >= m_vOverlabRatio.x)
	{
		if (!m_bDir)
		{
			XMStoreFloat3(&m_vLastDir, pSrcTransform->GetState(STATE::LOOK));
			m_vLastPos = pSrcTransform->GetPosition();
			m_bDir = true;
		}
		
		_float3 vPos = pSrcTransform->GetPosition();
		if (m_bOverLabLoop)
		{
			m_fCurOverLabSpeed += m_fOverLabSpeed * fTimeDelta;
		}
		
		PX_OVERLAP_DESC   pxOverLabDesc{};
		PX_OVERLAP_RESULT pxOverLapResult{};
		if (m_bOverLabMove)
		{
			XMStoreFloat3(&m_vLastPos, XMLoadFloat3(&m_vLastPos) + XMLoadFloat3(&m_vLastDir) * m_fOverLabSpeed * fTimeDelta);
			XMStoreFloat3(&vPos, XMLoadFloat3(&m_vLastPos));
		}

		pxOverLabDesc.tFilter = PX_QUERY_FILTER_DESC{ .iQueryMask = ETOUI(COLLISION_LAYER::PLAYER_HURTBOX) };
		pxOverLabDesc.tGeometry = PX_QUERY_GEOMETRY_DESC{ .eType = PX_QUERY_GEOMETRY_TYPE::SPHERE,.fRadius = m_fCurOverLabSpeed };
		pxOverLabDesc.tPose = PX_QUERY_POSE{ .vPosition = vPos };

		vPos = pxOverLabDesc.tPose.vPosition;
		
		auto pDbgLineRender = CGameInstance::Get().GetDbgLineRender();

		const auto vPreviousColor = pDbgLineRender->GetColor();
		const auto ePreviousDepthMode = pDbgLineRender->GetDepthMode();
		pDbgLineRender->SetColor({ 0.f, 1.f, 1.f, 1.f });
		pDbgLineRender->SetDepthTest(true);
		pDbgLineRender->AddSphere(m_fCurOverLabSpeed, XMMatrixTranslation(vPos.x, vPos.y, vPos.z));
		pDbgLineRender->SetColor(vPreviousColor);
		pDbgLineRender->SetDepthMode(ePreviousDepthMode);

		if (CGameInstance::Get().GetPhysXManager()->Overlap(pxOverLabDesc, pxOverLapResult))
		{
			if (pxOverLapResult.bHit)
			{
				//m_fDamage
				auto pTarget = CGameInstance::Get().GetGameObjectByHandleT<CPlayer>(pxOverLapResult.hGameObject);
				_float MonDamange = pMon->Get_Damage();
				pTarget->OnQueryHit(MonDamange);
				m_bAttRatio = true;
			}
		}
	}
	

}
void CBTAttackAnimation::ShakeCam(_float fRotRatio)
{
	if (m_fCamShakeRatio == 0.f)
		return;
	if (m_bCamShake && fRotRatio > m_fCamShakeRatio)
	{
		//카메라 쉐킷
		CGameInstance::Get().EventPublish(FRequestPlayerCameraShake
			{
			   1.f, // 강도 0 ~ 1
			   1.f, // 지속시간
			   15.f, // 초당 진동횟수
			});
		m_bCamShake = false;
	}
}
void CBTAttackAnimation::Abort()
{
	__super::Abort();
	Reset_CheckFlag();
	if (!m_Skills.empty())
	{
		for (auto& iter : m_Skills)
			iter.bTrigger = false;
	}
}
nlohmann::json CBTAttackAnimation::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "AttColRatio", m_vOverlabRatio);

	SaveJsonValue(j, "overlabLoop", m_bOverLabLoop);
	SaveJsonValue(j, "overlabSpeed", m_fOverLabSpeed);
	SaveJsonValue(j, "Intensive", m_fIntensive);
	SaveJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	SaveJsonEnum(j, "MOVE", m_eMove);
	SaveJsonValue(j, "CamShakeRatio", m_fCamShakeRatio);
	SaveJsonValue(j, "AttRadius", m_fAttRadius);
	SaveJsonValue(j, "OverlabMove", m_bOverLabMove);
	SaveJsonValue(j, "TriggerSkill", m_bTrigger);
	

	if (!m_Skills.empty())
	{
		uint32_t iMax{};
		iMax = m_Skills.size();
		SaveJsonValue(j, "NewSkillTableSize" , iMax);
		for (size_t i =0; i < m_Skills.size(); ++i )
		{
			SaveJsonEnum(j, "NewSkillType" + std::to_string(i), m_Skills[i].eSkill);
			SaveJsonValue(j, "NewSkillRatio" + std::to_string(i), m_Skills[i].fRatio);
		}
	}
	return j;
}
HRESULT CBTAttackAnimation::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "AttColRatio", m_vOverlabRatio);

	LoadJsonValue(j, "overlabLoop", m_bOverLabLoop);
	LoadJsonValue(j, "overlabSpeed", m_fOverLabSpeed);
	LoadJsonValue(j, "Intensive", m_fIntensive);
	LoadJsonValue(j, "MoveSpeed", m_Value.fSpeed);
	LoadJsonEnum(j, "MOVE", m_eMove);
	LoadJsonValue(j, "CamShakeRatio", m_fCamShakeRatio);
	LoadJsonValue(j, "AttRadius", m_fAttRadius);
	LoadJsonValue(j, "OverlabMove", m_bOverLabMove);
	LoadJsonValue(j, "TriggerSkill", m_bTrigger);

	uint32_t iMax{};
	if (LoadJsonValue(j, "NewSkillTableSize", iMax))
	{
		m_Skills.resize(iMax);
		for (size_t i = 0; i < m_Skills.size(); ++i)
		{
			LoadJsonEnum(j, "NewSkillType" + std::to_string(i), m_Skills[i].eSkill);
			LoadJsonValue(j, "NewSkillRatio" + std::to_string(i), m_Skills[i].fRatio);
		}
	}
	return S_OK;
}


void CBTAttackAnimation::OnEnter()
{
	m_bDir = false;
	__super::OnEnter();
	m_bAttRatio = m_bActiveSkill = false;
	m_bCamShake = true;
	m_fTime = 0.f;
	m_fCurOverLabSpeed = m_fAttRadius;

	if (!m_Skills.empty())
	{
		for (auto& iter : m_Skills)
			iter.bTrigger = false;
	}

}
void CBTAttackAnimation::OnExit(EVALUATE eResult)
{
	m_bDir = false;
}
_bool CBTAttackAnimation::ActiveTriggerSkill(ATTMON eAtt)
{
	auto pBT = Get_ComBT();
	if (nullptr == pBT) return false;

	auto pMonster = static_cast<CMonster*>(pBT->GetGameObject());
	if (nullptr == pMonster) return false;
	
	pMonster->Set_AttTable(eAtt, m_fSkillRatio);
	return true;
}
E::UPtr<CBTAttackAnimation> CBTAttackAnimation::Create()
{
	auto pInstance = E::ToUPtr(new CBTAttackAnimation{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTAttackAnimation");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTAttackAnimation::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTAttackAnimation{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTAttackAnimation");
		return nullptr;
	}

	return pInstance;
}
