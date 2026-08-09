#include "pch.h"
#include "BTAnimRoot.h"
#include "ComAnimator.h" 
#include "ComCharacterMotor.h"
#include "ComCharacterMoveIntent.h"
NS_USING(Client)

CBTAnimRoot::CBTAnimRoot()
{

}
CBTAnimRoot::CBTAnimRoot(const CBTAnimRoot& rhs) : CBTActionNode(rhs)
{

}

CBTAnimRoot::~CBTAnimRoot()
{
}
HRESULT CBTAnimRoot::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_eGroup = NODEGROUP::ANIMATION;
	return S_OK;
}
HRESULT CBTAnimRoot::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

void CBTAnimRoot::Update_Gui()
{
	BoolButton("ShowAnim: ", m_bShow);
	if (m_bShow)
	{
		const _string& Name = CGameInstance::Get().GetAnimName(m_Value.iAnimIndex, Get_Handle());
		ImGui::Text(Name.c_str());
	}
	if (ImGui::TreeNode("AnimRoot"))
	{
		BoolButton("Gravity : ", m_bGravity);
		if(m_bGravity)
			DragFloat("Gravity Value", m_fGravity);
		
		BoolButton("Enable Early : ", m_bEarly);
		if(m_bEarly)
			DragFloat("Early Ratio : ", m_fEarlyRatio);

		ImGui::DragFloat("Blend", &m_fBlend, 0.1f, 0.f, 1.f);

		BoolButton("Enable Ratio : ", m_bRatio);
		BoolButton("Loop Change : ", m_bLoop);

		DragFloat("StartRatio", m_fRatio.x);
		DragFloat("EndRatio", m_fRatio.y);

		ImGui::Text("SkillRatio");
		ImGui::DragFloat2("##SKRaito", reinterpret_cast<_float*>(&m_fSkillRatio), 0.f, 1.f);

		ImGui::Text("RotRatio");
		ImGui::DragFloat2("##RotRatio", reinterpret_cast<_float*>(&m_vRotRatio), 0.1f, 0.f, 1.f);

		ImGui::Text("AttMon Type");
		if (auto pBT = Get_ComBT())
		{
			if (auto pSrc = static_cast<CMonster*>(pBT->GetGameObject()))
			{
				if (ImGui::BeginCombo("##AttMon Typer", pSrc->Get_SkillName(m_eSkillType).c_str()))
				{
					for (uint32_t i = 0; i < ETOUI(ATTMON::END) + 1; ++i)
					{
						_string SkillName = pSrc->Get_SkillName(static_cast<ATTMON>(i));
						if (SkillName == "")
							continue;

						_bool bSelect = static_cast<int32_t>(m_eSkillType) == i;

						if (ImGui::Selectable(SkillName.data()))
							m_eSkillType = static_cast<ATTMON>(i);

						if (bSelect)
							ImGui::SetItemDefaultFocus();
					
					}
					ImGui::EndCombo();
				}
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::Button("Add To Start Flag"))
	{
		m_StartFlags.push_back(m_AddFlag);
		m_AddFlag.fRatio = 0;
		m_AddFlag.iFlag = 0;
	}
	if (ImGui::Button("Add To End Flag"))
	{
		m_EndFlags.push_back(m_AddFlag);
		m_AddFlag.fRatio = 0;
		m_AddFlag.iFlag = 0;
	}
	if (ImGui::TreeNode("Show Start Flag"))
	{
		uint32_t iNumIndex{ 0 };
		for (auto StartFlag = m_StartFlags.begin(); StartFlag != m_StartFlags.end(); ++StartFlag)
		{
			ImGui::PushID(iNumIndex);
			_string TypeName  = "SType" + std::to_string(iNumIndex) + " : ";
			_string RatioName = "SRatio" + std::to_string(iNumIndex) + " : ";
			_string FlagName  = "SFlag" + std::to_string(iNumIndex) + " : ";
			ImGui::Text(RatioName.c_str()); ImGui::SameLine();
			ImGui::DragFloat("##Ratio", &(*StartFlag).fRatio, 0.f, 1.f); ImGui::SameLine();

			ImGui::Text(FlagName.c_str()); ImGui::SameLine();
			Combo("##Flag", (*StartFlag).iFlag); ImGui::SameLine();

			ImGui::Text(TypeName.c_str()); ImGui::SameLine();
			Combo2("##Type", (*StartFlag).eType); ImGui::SameLine();

			if (ImGui::Button("Del"))
			{
				m_StartFlags.erase(StartFlag);
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
			++iNumIndex;
		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("Show End Flag"))
	{
		uint32_t iNumIndex{ 0 };
		for (auto EndFlag = m_EndFlags.begin(); EndFlag != m_EndFlags.end(); ++EndFlag)
		{
			ImGui::PushID(iNumIndex);
			_string TypeName =  "EType" + std::to_string(iNumIndex) + " : ";
			_string RatioName = "ERatio" + std::to_string(iNumIndex) + " : ";
			_string FlagName =  "EFlag" + std::to_string(iNumIndex) + " : ";

			ImGui::Text(RatioName.c_str()); ImGui::SameLine();
			ImGui::DragFloat("##Ratio", &(*EndFlag).fRatio, 0.01f,0.f, 1.f); ImGui::SameLine();

			ImGui::Text(FlagName.c_str()); ImGui::SameLine();
			Combo("##Flag", (*EndFlag).iFlag);  ImGui::SameLine();

			ImGui::Text(TypeName.c_str()); ImGui::SameLine();
			Combo2("##Type", (*EndFlag).eType);  ImGui::SameLine();

			if (ImGui::Button("Del"))
			{
				m_EndFlags.erase(EndFlag);
				ImGui::PopID();
				break;
			}
			ImGui::PopID();
			++iNumIndex;

		}
		ImGui::TreePop();
	}
	if (ImGui::TreeNode("SoundTableHelpme"))
	{
		AddSound();
		
		if (ImGui::TreeNode("SoundsValue"))
		{
			SoundTableValueList();
			ImGui::TreePop();
		}
		ImGui::TreePop();
		
	}

}
void CBTAnimRoot::Abort()
{
	m_bStart = true;
	m_iLoopCnt = 0;
	Reset_CheckFlag();
	auto pSoundManager = CGameInstance::Get().GetSoundManager();
	for (auto& iter : m_Sounds)
	{
		iter.fCurRatioTime = 0.f;
		iter.bPlayed = false;

		if (iter.SoundPlay.bLoop && iter.iSoundID != INVALID_SOUND_ID && pSoundManager->IsValidSound(iter.iSoundID))
		{
			pSoundManager->Stop(iter.iSoundID);
			iter.iSoundID = INVALID_SOUND_ID;
		}
	}
		
}
nlohmann::json CBTAnimRoot::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "RotRatio", m_vRotRatio);
	SaveJsonValue(j, "EnableRatio", m_bRatio);
	SaveJsonValue(j, "Loop", m_bLoop);
	SaveJsonValue(j, "Blend", m_fBlend);

	SaveJsonValue(j, "Gravity Value", m_fGravity);
	SaveJsonValue(j, "Gravity", m_bGravity);

	SaveJsonValue(j, "Early", m_bEarly);
	SaveJsonValue(j, "EarlyRatio", m_fEarlyRatio);
	SaveJsonEnum(j, "SkillType", m_eSkillType);
	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "SkillRatio", m_fSkillRatio);
	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "Ratio_TypeF2", m_fRatio);
	
	size_t iMaxSize = m_StartFlags.size();
	if (!m_StartFlags.empty())
	{
		SaveJsonValue(j, "StartRatioArraySize", iMaxSize);
		for (size_t i = 0; i < m_StartFlags.size(); ++i)
		{
			_string FlagTypeName = "EventStartFlagType" + std::to_string(i);
			_string FlagRatioName = "EventStartFlagRatio" + std::to_string(i);
			_string FlagValue = "EventStartFlagValue" + std::to_string(i);

			SaveJsonEnum(j, FlagTypeName, m_StartFlags[i].eType);
			SaveJsonValue(j, FlagRatioName, m_StartFlags[i].fRatio);
			SaveJsonValue(j, FlagValue, m_StartFlags[i].iFlag);
		}
	}

	iMaxSize = m_EndFlags.size();
	if (!m_EndFlags.empty())
	{
		SaveJsonValue(j, "EndRatioArraySize", iMaxSize);
		for (size_t i = 0; i < m_EndFlags.size(); ++i)
		{
			_string FlagTypeName  = "EventEndFlagType"  + std::to_string(i);
			_string FlagRatioName = "EventEndFlagRatio" + std::to_string(i);
			_string FlagValue     = "EventEndFlagValue" + std::to_string(i);

			SaveJsonEnum(j,  FlagTypeName,  m_EndFlags[i].eType);
			SaveJsonValue(j, FlagRatioName, m_EndFlags[i].fRatio);
			SaveJsonValue(j, FlagValue,     m_EndFlags[i].iFlag);
		}
	}
	
	if (!m_Sounds.empty())
	{
		uint32_t iMaxSize = m_Sounds.size();
		SaveJsonValue(j, "SoundTableSize", iMaxSize);
		for (size_t i=0; i< m_Sounds.size(); ++i)
		{

			JsonSaveLoadManager::SaveJsonTypeString(j, "SoundKey" + std::to_string(i), m_Sounds[i].SoundKey);
			SaveJsonValue(j, "PlaySoundRatio" + std::to_string(i), m_Sounds[i].fPlayRatio);
			SaveJsonValue(j, "3DSoundfMinDist" + std::to_string(i), m_Sounds[i].str3DSound.fMinDistance);
			SaveJsonValue(j, "3DSoundfMaxDist" + std::to_string(i), m_Sounds[i].str3DSound.fMaxDistance);
			SaveJsonValue(j, "SoundPlayVolume" + std::to_string(i), m_Sounds[i].SoundPlay.fVolume);
			SaveJsonValue(j, "SoundPlayPitch" + std::to_string(i), m_Sounds[i].SoundPlay.fPitch);
			SaveJsonValue(j, "SoundPlayLoop" + std::to_string(i), m_Sounds[i].SoundPlay.bLoop);
			SaveJsonValue(j, "PlaySoundOne" + std::to_string(i), m_Sounds[i].bOnlyOne);
		}
	}

	return j;
}
HRESULT CBTAnimRoot::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);

	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "RotRatio", m_vRotRatio);
	LoadJsonValue(j, "Gravity Value", m_fGravity);
	LoadJsonValue(j, "Gravity", m_bGravity);
	LoadJsonValue(j, "Early", m_bEarly);
	LoadJsonValue(j, "EarlyRatio", m_fEarlyRatio);
	LoadJsonValue(j, "EnableRatio", m_bRatio);
	LoadJsonValue(j, "Loop", m_bLoop);
	LoadJsonEnum(j, "SkillType", m_eSkillType);
	LoadJsonValue(j, "Blend", m_fBlend);
	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "SkillRatio", m_fSkillRatio);
	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "Ratio_TypeF2", m_fRatio);
	

	size_t iMaxSize = {};
	if (LoadJsonValue(j, "StartRatioArraySize", iMaxSize))
	{
		m_StartFlags.resize(iMaxSize);
		for (size_t i = 0; i < iMaxSize; ++i)
		{
			_string FlagTypeName = "EventStartFlagType" + std::to_string(i);
			_string FlagRatioName = "EventStartFlagRatio" + std::to_string(i);
			_string FlagValue = "EventStartFlagValue" + std::to_string(i);

			LoadJsonEnum(j, FlagTypeName, m_StartFlags[i].eType);
			LoadJsonValue(j, FlagRatioName, m_StartFlags[i].fRatio);
			LoadJsonValue(j, FlagValue, m_StartFlags[i].iFlag);
		}
	}

	if (LoadJsonValue(j, "EndRatioArraySize", iMaxSize))
	{
		m_EndFlags.resize(iMaxSize);
		for (size_t i = 0; i < iMaxSize; ++i)
		{
			_string FlagTypeName = "EventEndFlagType" + std::to_string(i);
			_string FlagRatioName = "EventEndFlagRatio" + std::to_string(i);
			_string FlagValue = "EventEndFlagValue" + std::to_string(i);

			LoadJsonEnum(j, FlagTypeName, m_EndFlags[i].eType);
			LoadJsonValue(j, FlagRatioName, m_EndFlags[i].fRatio);
			LoadJsonValue(j, FlagValue, m_EndFlags[i].iFlag);
		}
	}
	iMaxSize = {};
	if (LoadJsonValue(j, "SoundTableSize", iMaxSize))
	{
		m_Sounds.resize(iMaxSize);
		MONSOUND MonSound{};
		MonSound.str3DSound = SOUND_3D_DESC{ .eRolloff = SOUND_3D_ROLLOFF::LINEAR };
		MonSound.SoundPlay = SOUND_PLAY_DESC{ .sBusID = SOUND_BUS::SFX ,.iPriority = 64, };

		for (size_t i = 0; i < m_Sounds.size(); ++i)
		{
			JsonSaveLoadManager::LoadJsonTypeString(j, "SoundKey" + std::to_string(i), m_Sounds[i].SoundKey);
			LoadJsonValue(j, "PlaySoundRatio" + std::to_string(i), m_Sounds[i].fPlayRatio);
			LoadJsonValue(j, "3DSoundfMinDist" + std::to_string(i), m_Sounds[i].str3DSound.fMinDistance);
			LoadJsonValue(j, "3DSoundfMaxDist" + std::to_string(i), m_Sounds[i].str3DSound.fMaxDistance);
			LoadJsonValue(j, "SoundPlayVolume" + std::to_string(i), m_Sounds[i].SoundPlay.fVolume);
			LoadJsonValue(j, "SoundPlayPitch" + std::to_string(i), m_Sounds[i].SoundPlay.fPitch);
			LoadJsonValue(j, "SoundPlayLoop" + std::to_string(i), m_Sounds[i].SoundPlay.bLoop);
			LoadJsonValue(j, "PlaySoundOne" + std::to_string(i), m_Sounds[i].bOnlyOne);
			m_Sounds[i].str3DSound.eRolloff = SOUND_3D_ROLLOFF::LINEAR;
			m_Sounds[i].SoundPlay.sBusID = SOUND_BUS::SFX;
			m_Sounds[i].SoundPlay.iPriority = 64;
		}
	}

	return S_OK;
}
_bool CBTAnimRoot::Active_Skill()
{
	
	if (auto pBT = Get_ComBT())
	{
		if (m_eSkillType == ATTMON::END)
			return false;

		if (auto pSrc = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			if (pBT->Check_Flag(ETOUI(BTFLAG::ENDHIT)))
				pSrc->Skill_Finished();

			if (pBT->Check_Flag(ETOUI(BTFLAG::ATTACK)))
				return false;


			pSrc->Set_AttTable(m_eSkillType, m_fSkillRatio);
			return true;
		}
	}
	return false;
}
void CBTAnimRoot::EventFlagToRatio(_float fRatio)
{
	if (auto pBT = Get_ComBT())
	{
		for (auto& startFlag : m_StartFlags)
		{
			if (startFlag.bFlag == true)
				continue;

			if (fRatio >= startFlag.fRatio)
			{
				pBT->Set_Flag(startFlag.iFlag, startFlag.eType);
				startFlag.bFlag = true;
			}
		}
		for (auto& endFlag : m_EndFlags)
		{
			if (endFlag.bFlag == true)
				continue;

			if (fRatio >= endFlag.fRatio)
			{
				pBT->Set_Flag(endFlag.iFlag, endFlag.eType);
				endFlag.bFlag = true;
			}
		}
	}
}

void CBTAnimRoot::Gravity()
{
	
	if (!m_bGravity) return;

	if (auto pBT = Get_ComBT())
	{
		if (auto pSrc = pBT->GetGameObject())
		{
			if (auto pMotor = pSrc->GetComponent<CComCharacterMotor>("ComCharacterMotor"))
			{
				pMotor->SetGravity(m_fGravity);
			}
		}
	}
	
}
void CBTAnimRoot::Reset_CheckFlag()
{
	for (auto& startFlag : m_StartFlags)
		startFlag.bFlag = false;
	for (auto& endFlag : m_EndFlags)
		endFlag.bFlag = false;

}

void CBTAnimRoot::OnEnter()
{
	m_bStart = true;
	m_iLoopCnt = 0;
	Reset_CheckFlag();
	auto pSoundManager = CGameInstance::Get().GetSoundManager();
	for (auto& iter : m_Sounds)
	{
		iter.bPlayed = false;
		iter.fCurRatioTime = 0.f;
		if (iter.iSoundID != INVALID_SOUND_ID &&
			!pSoundManager->IsValidSound(iter.iSoundID))
		{
			iter.iSoundID = INVALID_SOUND_ID;
		}
	}
		
	auto pBT = Get_ComBT();

	if (!pBT) return;

	auto pOwner = pBT->GetGameObject();

	if (!pOwner) return;

	auto pAnimator = pOwner->GetComponent<CComAnimator>("ComCModelAnimator");

	if (!pAnimator) return;

	auto& animState = pAnimator->GetCurAnimState();
	

}

void CBTAnimRoot::OnExit(EVALUATE eResult)
{
	auto pSoundManager = CGameInstance::Get().GetSoundManager();

	for (auto& iter : m_Sounds)
	{
		if (iter.SoundPlay.bLoop &&
			iter.iSoundID != INVALID_SOUND_ID &&
			pSoundManager->IsValidSound(iter.iSoundID))
		{
			pSoundManager->Stop(iter.iSoundID);
			iter.iSoundID = INVALID_SOUND_ID;
		}
	}
}


void CBTAnimRoot::Combo(const _char* pName,uint32_t& iFlag)
{
	struct GuiView
	{
		uint32_t iValue{};
		const _char* pName{};
	};
#define X(name, value) value, #name,
	const GuiView Flags[] = { BTFLAG_M };
#undef X
	const _char* pPreview{};
	for (const auto& Flag : Flags)
	{
		if (Flag.iValue == iFlag)
		{
			pPreview = Flag.pName;
			break;
		}
	}
	if (ImGui::BeginCombo(pName, pPreview))
	{
		for (uint32_t i = 0; i < std::size(Flags); ++i)
		{
			uint32_t iFFlag = 1u << i;
			bool bSelect = iFlag == iFFlag;
			if (ImGui::Selectable(Flags[i].pName))
				iFlag = Flags[i].iValue;

			if (bSelect)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
}
void CBTAnimRoot::Combo2(const _char* pName, FLAGTYPE& eType)
{
	if (ImGui::BeginCombo(pName, MagicEnumToStringView(eType).data()))
	{
		for (uint32_t i = 0; i < ETOUI(FLAGTYPE::RESET) + 1; ++i)
		{
			_bool bSelect = eType == static_cast<FLAGTYPE>(i);
			if (ImGui::Selectable(MagicEnumToStringView(static_cast<FLAGTYPE>(i)).data()))
			{
				eType = static_cast<FLAGTYPE>(i);
			}
			if (bSelect)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
}
void CBTAnimRoot::Rotation(CComTransform* pTransform, CComCharacterMoveIntent* pMoveIntent, CGameObject* pTarget, _float fTimeDelta, _float fRotRatio)
{
	if (m_vRotRatio.x < fRotRatio && m_vRotRatio.y >= fRotRatio)
	{
		_float3 vFacingDirection{};
		XMStoreFloat3(&vFacingDirection, pTarget->GetTransform().GetState(STATE::POSITION) - pTransform->GetState(STATE::POSITION));
		const _float fTurnTime = std::max(m_Value.fTime, 0.001f);
		pMoveIntent->SetFacingIntent(vFacingDirection, 180.f / fTurnTime);
	}


}
void CBTAnimRoot::AddSound()
{
	if (ImGui::Button("Add Table"))
	{
		MONSOUND MonSound{};
		MonSound.fPlayRatio = 0.0f;
		MonSound.SoundKey = "";
		MonSound.str3DSound = SOUND_3D_DESC{ .fMinDistance = 10.f, .fMaxDistance = 30.f, .eRolloff = SOUND_3D_ROLLOFF::LINEAR };
		MonSound.SoundPlay = SOUND_PLAY_DESC{ .sBusID = SOUND_BUS::SFX ,.fVolume = 0.5f,.fPitch = 1.f,.iPriority = 64,.bLoop = false };
		m_Sounds.push_back(MonSound);
	}
}

void CBTAnimRoot::SoundTableValueList()
{
	if (auto pBT = Get_ComBT())
	{
		if (auto pSrc = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			int32_t iPopIndex{ 0 };
			for (auto iter = m_Sounds.begin(); iter != m_Sounds.end(); ++iter)
			{
				
				ImGui::PushID(iPopIndex);

				if (ImGui::Button((*iter).SoundKey == "" ? "NONAME" : (*iter).SoundKey.c_str()))
				{
					ImGui::OpenPopup("SoundPopup");
				}

				SoundPopUp((*iter),pSrc);

				ImGui::SameLine();
				if (ImGui::Button("Del"))
				{
					m_Sounds.erase(iter);
					ImGui::PopID();
					break;
				}

				ImGui::PopID();
				++iPopIndex;
			}
			
		}
	}


}

void CBTAnimRoot::SoundPopUp(MONSOUND& Sound, CMonster* Monster)
{
	if (!ImGui::BeginPopup("SoundPopup"))
		return;
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1,0,0,1 });
	Monster->Get_SoundKey(Sound.SoundKey);
	DragFloat("Interval Ratio", Sound.fPlayRatio);
	if (ImGui::Button("PlayOnlyOne : "))
		Sound.bOnlyOne = !Sound.bOnlyOne;
	ImGui::SameLine();
	ImGui::Text(Sound.bOnlyOne == true ? "TRUE" : "FALSE");


	ImGui::Separator();
	DragFloat("Sound_3D_DESC_fMinDist : ", Sound.str3DSound.fMinDistance);
	DragFloat("Sound_3D_DESC_fMaxDist : ", Sound.str3DSound.fMaxDistance);
	ImGui::Separator();
	
	DragFloat("SOUND_PLAY_DESC_Volume", Sound.SoundPlay.fVolume);
	DragFloat("SOUND_PLAY_DESC_Pitch", Sound.SoundPlay.fPitch);
	if (ImGui::Button("SOUND_PLAY_DESC_LOOP : "))
		Sound.SoundPlay.bLoop = !Sound.SoundPlay.bLoop;
	ImGui::SameLine();
	ImGui::Text(Sound.SoundPlay.bLoop == true ? "TRUE" : "FALSE");
	if (ImGui::Button("Close"))
		ImGui::CloseCurrentPopup();
	ImGui::PopStyleColor();
	ImGui::EndPopup();
}

void CBTAnimRoot::DragFloat(const _char* pName, _float& fValue)
{
	_string Name = "##" + _string(pName);
	ImGui::Text(pName);
	ImGui::DragFloat(Name.c_str(), &fValue,0.1f,0.f,1.f);
}
void CBTAnimRoot::BoolButton(const _char* pName, _bool& bButton)
{
	if (ImGui::Button(pName))
		bButton = !bButton;
	ImGui::SameLine();
	ImGui::Text(bButton == true ? "TRUE" : "FALSE");
}
void CBTAnimRoot::Play_Sound(_float fTimeDelta)
{
	if (m_Sounds.empty())
		return;

	auto pBT = Get_ComBT();

	if (!pBT) return;

	auto pOwner = static_cast<CMonster*>(pBT->GetGameObject());

	if (!pOwner) return;
	auto pSoundManager = CGameInstance::Get().GetSoundManager();
	for (auto& iter : m_Sounds)
	{
		iter.fCurRatioTime += fTimeDelta;
		const _bool bPlaying = iter.iSoundID != INVALID_SOUND_ID && pSoundManager->IsValidSound(iter.iSoundID) &&
			pSoundManager->IsPlaying(iter.iSoundID);
	
		if (bPlaying)
			continue;
		if (iter.bOnlyOne &&iter.bPlayed)
			continue;
		if (iter.fPlayRatio > 0.f && iter.fCurRatioTime < iter.fPlayRatio)
			continue;
		
		iter.iSoundID  = pOwner->Play_Sound(iter);
			if (iter.iSoundID != INVALID_SOUND_ID)
			{
				iter.bPlayed = true;
				iter.fCurRatioTime = 0.f;
			}
	}
}
E::UPtr<CBTAnimRoot> CBTAnimRoot::Create()
{
	auto pInstance = E::ToUPtr(new CBTAnimRoot{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTAnimRoot");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTAnimRoot::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTAnimRoot{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTAnimRoot");
		return nullptr;
	}

	return pInstance;
}
