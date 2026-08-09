#include "pch.h"
#include "UIController.h"
#include "UIManager.h"
#include "HPBar.h"
#include "SpellMeter.h"
#include "TextBox.h"
#include "Button.h"
#include "Cursor.h"
#include "Monster.h"
#include "MiniMap.h"
#include "ClientEvents.h"

NS_USING(Client)

CUIController::CUIController()
{
}

CUIController::~CUIController()
{
}

HRESULT CUIController::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CUIController::Initialize(void* pArg)
{
	auto		pDesc = static_cast<GAMEOBJECT_DESC*>(pArg);

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	CreatePlayScreen();
	BindMiniMap();
	SubscribeQuestUIEvents();
	ActivePlayScreen = true;
	
	{
		auto clientSize = CGameInstance::Get().GetClientScreenSize();
		std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

		CCursor::UIOBJECT_DESC Desc{};

		Desc.sObjectTag = "Cursor";
		Desc.Name = "Cursor";
		Desc.fSizeX = 64.f;
		Desc.fSizeY = 64.f;
		Desc.fX = clientSize.x * 0.5f;
		Desc.fY = clientSize.y * 0.5f;
		Desc.fAlpha = 1.f;
		Desc.UIType = ETOUI(UI_TYPE::CURSOR);
		Desc.ResWeight = 1000;

		m_Cursor = E::CGameInstance::Get().AddGameObjectToLayer(currentLevel, "Prototype_GameObject_Cursor", "Layer_UI", &Desc);

		/****페이드인*****/
		//PlayFadeOutDelete(GET_SINGLE(UIManager)->LoadPrefab("BlackBG").front(), 1.f, 2.f);
	}

	return S_OK;
}

void CUIController::PriorityUpdate(E::_float fTimeDelta)
{	
}

void CUIController::Update(E::_float fTimeDelta)
{
	BindMiniMap();
	ApplyPendingQuestUIGroups();

	if (!CursorCreate)
	{

		auto clientSize = CGameInstance::Get().GetClientScreenSize();
		std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();
		
		CCursor::UIOBJECT_DESC Desc{};
		
		Desc.sObjectTag = "Cursor";
		Desc.Name = "Cursor";
		Desc.fSizeX = 64.f;
		Desc.fSizeY = 64.f;
		Desc.fX = clientSize.x * 0.5f;
		Desc.fY = clientSize.y * 0.5f;
		Desc.fAlpha = 1.f;
		Desc.UIType = ETOUI(UI_TYPE::CURSOR);
		Desc.ResWeight = 1000;
		
		m_Cursor = E::CGameInstance::Get().AddGameObjectToLayer(currentLevel, "Prototype_GameObject_Cursor", "Layer_UI", &Desc);

		CursorCreate = true;

	}

	// ************** 플레이어 HP
	if (E::CGameInstance::Get().KeyDown(DIK_9))
	{
		AddHP(-200.f);
	}

	// ************** 플레이어 Finisher
	if (E::CGameInstance::Get().KeyDown(DIK_8))
	{
		AddFinisher(10.f);
	}
	if (E::CGameInstance::Get().KeyDown(DIK_7))
	{
		AddFinisher(-10.f);
	}

	// ************** 스펠슬롯
	if (E::CGameInstance::Get().KeyDown(DIK_1))
	{
		UseSpell(1);
	}
	else if (E::CGameInstance::Get().KeyDown(DIK_2))
	{
		UseSpell(2);
	}
	else if (E::CGameInstance::Get().KeyDown(DIK_3))
	{
		UseSpell(3);
	}
	else if (E::CGameInstance::Get().KeyDown(DIK_4))
	{
		UseSpell(4);
	}

	// ************** 포션
	if (E::CGameInstance::Get().KeyDown(DIK_NEXT))
	{
		UsePotion();
		E::CGameInstance::Get().GetSoundManager()->Play2D("./Resources/SampleClient/Sound/UI/Potion.wav", SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::UI,
			.fVolume = 1.f,
			.fPitch = 1.f,
			.iPriority = 64,
			.bLoop = false
		});
	}
	// ************** 스펠슬롯
	if (E::CGameInstance::Get().KeyDown(DIK_B))
	{
		ActiveShortCutSlot ? ActiveShortCutSlot = false : ActiveShortCutSlot = true;
		if (ActiveShortCutSlot)
		{
			CreateSpellType();
			SafeGetOBJ(m_PotionCount)->SetActive(false);
		}
		else
		{
			DeleteSpellType();
			SafeGetOBJ(m_PotionCount)->SetActive(true);
		}
			
	}

	// 몬스터 HP 감송
	if (E::CGameInstance::Get().KeyDown(DIK_6))
		AddMonsterHP(-30.f);

	// 죽는 화면
	//if (E::CGameInstance::Get().KeyDown(DIK_0) && !m_isCreateDeathScene)
	//{
	//	m_isCreateDeathScene = true;
	//	CreateDeathScene();
	//}
	

	/****************필수********************/
	if (m_bMonsterHP)
	{
		CreateMonsterHP();
		m_bMonsterHP = false;
	}

	UpdateMonsterHP();
}

void CUIController::LateUpdate(E::_float fTimeDelta)
{
}

HRESULT CUIController::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

void CUIController::CreatePlayScreen()
{
	/*******플레이어 체력*******/
	m_PlayerHP = GET_SINGLE(UIManager)->LoadPrefab("PlayerHP").front();
	static_cast<CHPBar*>(SafeGetOBJ(m_PlayerHP))->SetMaxFill(500);
	static_cast<CHPBar*>(SafeGetOBJ(m_PlayerHP))->SetCurrentFill(500);
	/*******피니셔*******/
	m_Finisher[0] = GET_SINGLE(UIManager)->LoadPrefab("Finisher1").front();
	m_Finisher[1] = GET_SINGLE(UIManager)->LoadPrefab("Finisher2").front();
	m_Finisher[2] = GET_SINGLE(UIManager)->LoadPrefab("Finisher3").front();
	m_FinisherIcon = GET_SINGLE(UIManager)->LoadPrefab("FinisherIcon").front();
	for (auto pHandle : m_Finisher)
	{
		static_cast<CHPBar*>(SafeGetOBJ(pHandle))->SetAmount(0.f);
	}
	SetFinisher(0.f);

	/*******스펠슬롯*******/
	m_SpellSlot[0] = GET_SINGLE(UIManager)->LoadPrefab("SpellSlot1").front();
	m_SpellSlot[1] = GET_SINGLE(UIManager)->LoadPrefab("SpellSlot2").front();
	m_SpellSlot[2] = GET_SINGLE(UIManager)->LoadPrefab("SpellSlot3").front();
	m_SpellSlot[3] = GET_SINGLE(UIManager)->LoadPrefab("SpellSlot4").front();

	for (auto phandle : m_SpellSlot)
	{
		static_cast<CSpellMeter*>(SafeGetOBJ(phandle))->SetSpellType(ETOUI(SPELL_TYPE::NONE));
		static_cast<CSpellMeter*>(SafeGetOBJ(phandle))->SetResTagDirtyFlag(true);
	}
	static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[0]))->SetSpellType(ETOUI(SPELL_TYPE::BOMBARDA));

	/*******포션 개수*******/
	m_PotionCount = GET_SINGLE(UIManager)->LoadPrefab("PotionCount").front();

	/*******정적 유아이*******/
	m_PlaySceneStatic = GET_SINGLE(UIManager)->LoadPrefab("StaticPlayScreen");
}

_bool CUIController::SetQuestUIGroupActive(
	QUEST_UI_GROUP group, _bool active)
{
	const size_t index = static_cast<size_t>(group);
	if (group == QUEST_UI_GROUP::NONE ||
		group == QUEST_UI_GROUP::END ||
		index >= QUEST_UI_GROUP_COUNT)
		return false;

	if (m_QuestUIGroupStates[index] != active)
	{
		m_QuestUIGroupStates[index] = active;
		m_QuestUIGroupDirty[index] = true;
	}

	ApplyPendingQuestUIGroups();
	return true;
}

void CUIController::BindMiniMap()
{
	if (m_hMiniMap && E::CGameInstance::Get().
		GetGameObjectByHandleT<CMiniMap>(*m_hMiniMap))
		return;

	m_hMiniMap = std::nullopt;
	std::vector<CHandle> pendingHandles = m_PlaySceneStatic;
	for (size_t i = 0; i < pendingHandles.size(); ++i)
	{
		const CHandle handle = pendingHandles[i];
		if (E::CGameInstance::Get().
			GetGameObjectByHandleT<CMiniMap>(handle))
		{
			m_hMiniMap = handle;
			for (size_t groupIndex = 1;
				groupIndex < QUEST_UI_GROUP_COUNT; ++groupIndex)
			{
				m_QuestUIGroupDirty[groupIndex] = true;
			}
			return;
		}

		auto* uiObject = E::CGameInstance::Get().
			GetGameObjectByHandleT<CUIObject>(handle);
		if (!uiObject)
			continue;

		const auto& children = uiObject->GetChildren();
		pendingHandles.insert(
			pendingHandles.end(), children.begin(), children.end());
	}
}

void CUIController::SubscribeQuestUIEvents()
{
	if (m_iQuestUIListenerID != 0)
		return;

	m_iQuestUIListenerID = E::CGameInstance::Get().
		EventSubscribe<FQuestUIGroupChanged>(
			GetHandle(),
			[this](const FQuestUIGroupChanged& event)
			{
				SetQuestUIGroupActive(event.Group, event.Active);
			});
}

void CUIController::ApplyPendingQuestUIGroups()
{
	if (!m_hMiniMap)
		return;

	auto* miniMap = E::CGameInstance::Get().
		GetGameObjectByHandleT<CMiniMap>(*m_hMiniMap);
	if (!miniMap)
	{
		m_hMiniMap = std::nullopt;
		return;
	}

	for (size_t index = 1; index < QUEST_UI_GROUP_COUNT; ++index)
	{
		if (!m_QuestUIGroupDirty[index])
			continue;

		const auto group = static_cast<QUEST_UI_GROUP>(index);
		if (miniMap->SetContentGroupActive(
			group, m_QuestUIGroupStates[index]))
		{
			m_QuestUIGroupDirty[index] = false;
		}
	}
}

void CUIController::CreateSpellType()
{
	/********스펠슬롯**********/
	m_SpellBTNs = GET_SINGLE(UIManager)->LoadPrefab("OnlySpellBTN");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[0]))->SetResTag("TEX_UI_T_spellmeter_ArrestoMomentum_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[1]))->SetResTag("TEX_UI_T_spellmeter_Glacius_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[2]))->SetResTag("TEX_UI_T_spellmeter_Levioso_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[3]))->SetResTag("TEX_UI_T_spellmeter_TransformationOverlandOverlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[4]))->SetResTag("TEX_UI_T_spellmeter_Accio_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[5]))->SetResTag("TEX_UI_T_spellmeter_Depulso_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[6]))->SetResTag("TEX_UI_T_spellmeter_Descendo_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[7]))->SetResTag("TEX_UI_T_spellmeter_Flipendo_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[8]))->SetResTag("TEX_UI_T_spellmeter_Confringo_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[9]))->SetResTag("TEX_UI_T_spellmeter_Diffindo_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[10]))->SetResTag("TEX_UI_T_spellmeter_Expelliarmus_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[11]))->SetResTag("TEX_UI_T_spellmeter_Bombarda_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[12]))->SetResTag("TEX_UI_T_spellmeter_Incendio_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[13]))->SetResTag("TEX_UI_T_spellmeter_Disillusionment_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[14]))->SetResTag("TEX_UI_T_spellmeter_Lumos_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[15]))->SetResTag("TEX_UI_T_spellmeter_Reparo_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[16]))->SetResTag("TEX_UI_T_spellmeter_WingardiumLeviosa_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[17]))->SetResTag("TEX_UI_T_spellmeter_AvadaKedavra_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[18]))->SetResTag("TEX_UI_T_spellmeter_Crucio_Overlay");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[19]))->SetResTag("TEX_UI_T_spellmeter_Imperio_Overlay");

	/*********비디오 패스**********/
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[0]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_ArrestoMomentum.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[1]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Glacius.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[2]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Levioso.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[3]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Transformation.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[4]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Accio.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[5]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Depulso.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[6]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Descendo.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[7]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Flipendo.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[8]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Confringo.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[9]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Diffindo.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[10]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Expelliarmus.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[11]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Bombarda.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[12]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Incendio.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[13]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Disillusionment.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[14]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Lumos.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[15]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Reparo.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[16]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_WingardiumLeviosa.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[17]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_AvadaKedavra.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[18]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Crucio.avi");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[19]))->SetVideoPath(L"./Resources/SampleClient/Textures/UI/Video/FMV_Imperio.avi");

	/*********디스크립션 json 이름**********/
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[0]))->SetDescJsonname("AristoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[1]))->SetDescJsonname("GlaciusDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[2]))->SetDescJsonname("LeviosoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[3]))->SetDescJsonname("TransformDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[4]))->SetDescJsonname("AssioDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[5]))->SetDescJsonname("DepulsoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[6]))->SetDescJsonname("DescendoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[7]))->SetDescJsonname("FlipendoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[8]))->SetDescJsonname("ConfringoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[9]))->SetDescJsonname("DiffindoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[10]))->SetDescJsonname("ExpelliarmusDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[11]))->SetDescJsonname("BombardaDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[12]))->SetDescJsonname("IncendioDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[13]))->SetDescJsonname("DisillDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[14]))->SetDescJsonname("LumosDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[15]))->SetDescJsonname("ReparoDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[16]))->SetDescJsonname("WingardiumDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[17]))->SetDescJsonname("AvadaKedavraDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[18]))->SetDescJsonname("CrucioDesc");
	static_cast<CButton*>(SafeGetOBJ(m_SpellBTNs[19]))->SetDescJsonname("ImperioDesc"); 

	/********단축키슬롯**********/
	m_SpellShortCutKeySlot[0] = GET_SINGLE(UIManager)->LoadPrefab("ShortCut1").front();
	m_SpellShortCutKeySlot[1] = GET_SINGLE(UIManager)->LoadPrefab("ShortCut2").front();
	m_SpellShortCutKeySlot[2] = GET_SINGLE(UIManager)->LoadPrefab("ShortCut3").front();
	m_SpellShortCutKeySlot[3] = GET_SINGLE(UIManager)->LoadPrefab("ShortCut4").front();
	for (int i = 0; i < 4; i++)
	{
		uint32_t slotspellType = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[i]))->GetSpellType();
		static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellShortCutKeySlot[i]))->SetSpellType(slotspellType + ETOUI(SPELL_TYPE::B_NONE));
	}
	//static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellShortCutKeySlot[0]))->SetSpellType(ETOUI(SPELL_TYPE::B_BOMBARDA));

	m_SpellSlotStatic = GET_SINGLE(UIManager)->LoadPrefab("SpellSlotStatic");

	E::CGameInstance::Get().GetSoundManager()->Play2D("./Resources/SampleClient/Sound/UI/SpellOpen.wav", SOUND_PLAY_DESC{
		.sBusID = SOUND_BUS::UI,
		.fVolume = 0.5f,
		.fPitch = 1.f,
		.iPriority = 64,
		.bLoop = false
		});

	E::CGameInstance::Get().SetMouseFix(false);
	SafeGetOBJ(*m_Cursor)->SetAlpha(1.f);
}

void CUIController::DeleteSpellType()
{
	for (auto hBtn : m_SpellBTNs)
	{
		PlayScaleAlphaDownDelete(hBtn);
	}
	for (auto hBG : m_SpellSlotStatic)
	{
		PlayFadeOutDelete(hBG);
	}


	for (int i = 0; i < 4; i++)
	{
		CSpellMeter* pSpellMeter = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellShortCutKeySlot[i]));
		CSpellMeter* pSpellSlot = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[i]));

		if(pSpellSlot->GetSpellType() != pSpellMeter->GetSpellType() - ETOUI(SPELL_TYPE::B_NONE))
			SetSpellType(i + 1, pSpellMeter->GetSpellType() - ETOUI(SPELL_TYPE::B_NONE));
		PlayScaleAlphaDownDelete(m_SpellShortCutKeySlot[i]);
	}

	E::CGameInstance::Get().SetMouseFix(true);
	SafeGetOBJ(*m_Cursor)->SetAlpha(0.f);

	E::CGameInstance::Get().GetSoundManager()->Play2D("./Resources/SampleClient/Sound/UI/SpellClose.wav", SOUND_PLAY_DESC{
	.sBusID = SOUND_BUS::UI,
	.fVolume = 0.3f,
	.fPitch = 1.f,
	.iPriority = 64,
	.bLoop = false
		});
}

void CUIController::CreateDeathScene()
{
	m_isCreateDeathScene = true;

	m_Desolve = GET_SINGLE(UIManager)->LoadPrefab("Desolve").front();
	m_DeathDivider = GET_SINGLE(UIManager)->LoadPrefab("DeathDivider").front();
	m_DeathTxt = GET_SINGLE(UIManager)->LoadPrefab("DeathSceneTxt");

	PlayDividerUPWidth(m_DeathDivider);

	for (int i = 0; i < m_DeathTxt.size(); i++)
	{
		PlayAlphaUP(m_DeathTxt[i], 3.f - i * 0.5f, 1.f);
	}
		
	m_BeathButton[0] = GET_SINGLE(UIManager)->LoadPrefab("ButtonText1").front();
	m_BeathButton[1] = GET_SINGLE(UIManager)->LoadPrefab("ButtonText2").front();
	m_BeathButton[2] = GET_SINGLE(UIManager)->LoadPrefab("ButtonText3").front();
	PlayAlphaUP(m_BeathButton[0], 3.5f, 1.f);
	PlayAlphaUP(m_BeathButton[1], 3.75f, 1.f);
	PlayAlphaUP(m_BeathButton[2], 4.f, 1.f);

	m_GameOverMask = GET_SINGLE(UIManager)->LoadPrefab("GameOverMask").front();
	PlayAlphaUP(m_GameOverMask, 1.7f, 1.8f);
	GetSafeUI(m_GameOverMask)->SetSize({1024.f, 700.f});

	for (auto hUI : m_BeathButton)
	{
		SafeGetOBJ(hUI)->OnHoverEnter = GET_SINGLE(UIManager)->GetAction("TxtButtonScaleUp");
		SafeGetOBJ(hUI)->OnHoverExit = GET_SINGLE(UIManager)->GetAction("TxtButtonScaleDown");
		SafeGetOBJ(SafeGetOBJ(hUI)->GetChildren().front())->OnHoverEnter = GET_SINGLE(UIManager)->GetAction("TxtButtonColorUp");
		SafeGetOBJ(SafeGetOBJ(hUI)->GetChildren().front())->OnHoverExit = GET_SINGLE(UIManager)->GetAction("TxtButtonColorDown");
	}
	SafeGetOBJ(SafeGetOBJ(m_BeathButton[0])->GetChildren().front())->OnClickedAction = GET_SINGLE(UIManager)->GetFunc("ClearDeathScene");

	//SafeGetOBJ(m_PotionCount)->GetUIInfo().Color = { 0.f, 0.f, 0.f };
	PlayFadeOutOnly(m_PotionCount);
	E::CGameInstance::Get().SetMouseFix(false);
	SafeGetOBJ(*m_Cursor)->SetAlpha(1.f);

	E::CGameInstance::Get().GetSoundManager()->Play2D("./Resources/SampleClient/Sound/UI/Death.wav", SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::UI,
			.fVolume = 0.5f,
			.fPitch = 1.f,
			.iPriority = 64,
			.bLoop = false
		});

	/*********텍스트 안보이게**********/
	if (std::nullopt != m_MonsterHP && nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_MonsterHP))
		PlayFadeOutOnly(*m_MonsterHP);
}

void CUIController::SetHPMax(_float maxHP)
{
	if (nullptr != SafeGetOBJ(m_PlayerHP))
	{
		static_cast<CHPBar*>(SafeGetOBJ(m_PlayerHP))->SetMaxFill(maxHP);
	}
}


void CUIController::AddHP(_float amoutFill)
{
	if (nullptr != SafeGetOBJ(m_PlayerHP))
	{
		static_cast<CHPBar*>(SafeGetOBJ(m_PlayerHP))->AddFill(amoutFill);
	}
}

void CUIController::AddFinisher(_float amountFill)
{
	m_FinisherAmount += amountFill;
	SetFinisher(m_FinisherAmount);
}

void CUIController::SetFinisher(_float amountFill)
{
	m_FinisherAmount = amountFill;
	m_FinisherAmount = std::clamp(m_FinisherAmount, 0.0f, 100.f);
	if (nullptr != SafeGetOBJ(m_Finisher[0]))
	{
		static_cast<CHPBar*>(SafeGetOBJ(m_Finisher[0]))->SetCurrentFill(std::min(100.f, m_FinisherAmount * 10.f / 3.f));
		static_cast<CHPBar*>(SafeGetOBJ(m_Finisher[1]))->SetCurrentFill(std::min(100.f, (m_FinisherAmount - 100.f / 3.f) * 10.f / 3.f));
		static_cast<CHPBar*>(SafeGetOBJ(m_Finisher[2]))->SetCurrentFill(std::min(100.f, (m_FinisherAmount - 200.f / 3.f) * 10.f / 3.f));
	}
}

void CUIController::SetSpellType(uint32_t SlotNumber, uint32_t SpellType)
{
	CHandle SpellEffect = GET_SINGLE(UIManager)->LoadPrefab("SpellChoiceEffect").front();
	switch (SlotNumber)
	{
	case 1:
		{
			static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[0]))->SetSpellType(SpellType);
			_float2 pos = SafeGetOBJ(m_SpellSlot[0])->GetPos();
			SafeGetOBJ(SpellEffect)->SetPos(pos);
			break;
		}
	case 2:
		{
			static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[1]))->SetSpellType(SpellType);
			_float2 pos = SafeGetOBJ(m_SpellSlot[1])->GetPos();
			SafeGetOBJ(SpellEffect)->SetPos(pos);
			break;
		}
	case 3:
		{
			static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[2]))->SetSpellType(SpellType);
			_float2 pos = SafeGetOBJ(m_SpellSlot[2])->GetPos();
			SafeGetOBJ(SpellEffect)->SetPos(pos);
			break;
		}
	case 4:
		{
			static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[3]))->SetSpellType(SpellType);
			_float2 pos = SafeGetOBJ(m_SpellSlot[3])->GetPos();
			SafeGetOBJ(SpellEffect)->SetPos(pos);
			break;
		}
	defualt:
		break;
	}

}

uint32_t CUIController::GetSpellType(uint32_t SlotNumber)
{
	uint32_t spellType = 0;
	switch (SlotNumber)
	{
	case 1:
		spellType = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[0]))->GetSpellType();
		break;
	case 2:
		spellType = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[1]))->GetSpellType();
		break;
	case 3:
		spellType = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[2]))->GetSpellType();
		break;
	case 4:
		spellType = static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[3]))->GetSpellType();
		break;
	defualt:
		break;
	}

	return spellType;
}

void CUIController::UseSpell(uint32_t SlotNumber)
{
	switch (SlotNumber)
	{
	case 1:
		static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[0]))->StartCooldown();
		break;
	case 2:
		static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[1]))->StartCooldown();
		break;
	case 3:
		static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[2]))->StartCooldown();
		break;
	case 4:
		static_cast<CSpellMeter*>(SafeGetOBJ(m_SpellSlot[3]))->StartCooldown();
		break;
	defualt:
		break;
	}
}

void CUIController::SetPotionCount(_float cnt)
{
	m_iPotionCNT = cnt;
	m_iPotionCNT = std::clamp(m_iPotionCNT, 0, 99);
	if (m_iPotionCNT < 10)
	{
		static_cast<CTextBox*>(SafeGetOBJ(m_PotionCount))->SetwText(L"  " + std::to_wstring(m_iPotionCNT));
	}
	else if (nullptr != SafeGetOBJ(m_PotionCount))
	{
		static_cast<CTextBox*>(SafeGetOBJ(m_PotionCount))->SetwText(std::to_wstring(m_iPotionCNT));
	}
}

void CUIController::AddPotionCount(_float cnt)
{
	m_iPotionCNT += cnt;
	m_iPotionCNT = std::clamp(m_iPotionCNT, 0, 99);
	if (nullptr != SafeGetOBJ(m_PotionCount))
	{
		static_cast<CTextBox*>(SafeGetOBJ(m_PotionCount))->SetwText(std::to_wstring(m_iPotionCNT));
	}
}

void CUIController::UsePotion()
{
	if (m_iPotionCNT <= 0)
		return;

	AddPotionCount(-1);
	AddHP(400.f);
}

void CUIController::TargetMonsterHP(CHandle monsterHandle)
{
	m_ReserveTargetHandle = monsterHandle;
	if (m_MonsterHP != std::nullopt && nullptr != SafeGetOBJ(*m_MonsterHP))
	{
		PlayMonsterHPDeleteCreate(*m_MonsterHP);
	}
	else
	{
		m_bMonsterHP = true;
	}
}

void CUIController::CreateMonsterHP()
{
	m_TargetHandle = m_ReserveTargetHandle;

	if (m_TargetHandle == std::nullopt || nullptr == E::CGameInstance::Get().GetGameObjectByHandleT<CMonster>(*m_TargetHandle))
		return;

	auto* pMonster = E::CGameInstance::Get().GetGameObjectByHandleT<CMonster>(*m_TargetHandle);
	if (std::nullopt == m_TargetHandle || nullptr == pMonster)
		return;

	if (pMonster->Get_CurrentHp() <= 0.f)
		return;

	m_MonsterHP = GET_SINGLE(UIManager)->LoadPrefab("MonsterHP").front();
}

void CUIController::UpdateMonsterHP()
{
	if (m_TargetHandle != std::nullopt && nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CMonster>(*m_TargetHandle))
	{
		auto* pMonster = E::CGameInstance::Get().GetGameObjectByHandleT<CMonster>(*m_TargetHandle);
		
		if (m_MonsterHP != std::nullopt && nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_MonsterHP))
		{
			static_cast<CHPBar*>(GetSafeUI(*m_MonsterHP))->SetMaxFill(static_cast<_float>(pMonster->Get_MaxHp()));
			static_cast<CHPBar*>(GetSafeUI(*m_MonsterHP))->SetCurrentFill(static_cast<_float>(pMonster->Get_CurrentHp()));
		}

	}
	else {
		if (m_MonsterHP != std::nullopt && nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_MonsterHP))
		{	
			static_cast<CHPBar*>(GetSafeUI(*m_MonsterHP))->SetMaxFill(static_cast<_float>(100.f));
			static_cast<CHPBar*>(GetSafeUI(*m_MonsterHP))->SetCurrentFill(static_cast<_float>(0.f));
		}
	}
}

void CUIController::DeleteMonsterHP()
{
	if(m_MonsterHP != std::nullopt && E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_MonsterHP))
	{
		const CHandle hMonsterHP = *m_MonsterHP;
		m_bMonsterHP = false;
		m_MonsterHP = std::nullopt;
		PlayMonsterHPDelete(hMonsterHP);
	}
}

void CUIController::AddMonsterHP(_float fill)
{
	if (std::nullopt == m_MonsterHP || nullptr == E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_MonsterHP))
		return;
	static_cast<CHPBar*>(SafeGetOBJ(*m_MonsterHP))->AddFill(fill);
}

void CUIController::ClearDeathScene()
{
	GET_SINGLE(UIManager)->CreateFadeInSceneChange(0.f, 1.f, LEVEL::BOSS_CHARLES_ROOKWOOD);

	//PlayFadeOutDelete(m_Desolve);
	//PlayFadeOutDelete(m_DeathDivider);
	//PlayFadeOutDelete(m_DeathTxt[0]);
	//PlayFadeOutDelete(m_DeathTxt[1]);
	//
	//for (auto hUI : m_BeathButton)
	//{
	//	PlayFadeOutDelete(hUI);
	//}
	//
	////SafeGetOBJ(m_PotionCount)->GetUIInfo().Color = {1.f, 1.f, 1.f};
	//PlayFadeInOnly(m_PotionCount);
	//
	//PlayFadeOutDelete(m_GameOverMask);
	//m_isCreateDeathScene = false;

	if(std::nullopt != m_MonsterHP && nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_MonsterHP))
		PlayFadeInOnly(*m_MonsterHP);
	E::CGameInstance::Get().SetMouseFix(true);
	SafeGetOBJ(*m_Cursor)->SetAlpha(0.f);
}

E::CUIObject* CUIController::SafeGetOBJ(CHandle pHandle)
{
	if (nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(pHandle))
		return E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(pHandle);

	return nullptr;
}

void CUIController::PlayScaleAlphaDownDelete(CHandle pHandle)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float scaleRatio = pBtn->GetScaleRatio();
	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(scaleRatio, 0.f, 0.2f,
		[pBtn](float currentValue) {
			pBtn->SetScaleRatio(currentValue);
			pBtn->CalcUICoord();
		}, [pHandle]() {
			if (auto pObj = GetSafeUI(pHandle)) GET_SINGLE(UIManager)->DeleteUIRecursive(pHandle);
			}, EEaseType::EaseOutQuad);

		pTween->PlayTween(Alpah, 0.f, 0.2f,
			[pBtn](float currentValue) {
				pBtn->SetAlpha(currentValue);
				pBtn->CalcUICoord();
			}, nullptr, EEaseType::EaseOutQuad);
}

void CUIController::PlayFadeOutDelete(CHandle pHandle, float delaytime, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(1.f, 0.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, [pHandle]() {
			if (auto pObj = GetSafeUI(pHandle)) GET_SINGLE(UIManager)->DeleteUIRecursive(pHandle);
			}, EEaseType::EaseOutQuad, delaytime);
}

void CUIController::PlayFadeOutOnly(CHandle pHandle)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(1.f, 0.f, 1.f,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, nullptr, EEaseType::EaseOutQuad, 1.f);
}

void CUIController::PlayFadeInOnly(CHandle pHandle)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(0.f, 1.f, 0.5f,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, nullptr, EEaseType::EaseOutQuad, 0.2f);
}

void CUIController::PlayMonsterHPDelete(CHandle pHandle)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);
	_float scaleRatio = pBtn->GetScaleRatio();
	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(scaleRatio, 0.5f, 0.2f,
		[pBtn](float currentValue) {
			pBtn->SetScaleRatio(currentValue);
			pBtn->CalcUICoord();
		}, nullptr, EEaseType::EaseOutQuad);

	pTween->PlayTween(Alpah, 0.f, 0.2f,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
			pBtn->CalcUICoord();
		}, [pHandle, this]() {
			GET_SINGLE(UIManager)->DeleteUIRecursive(pHandle);
			}, EEaseType::EaseOutQuad);
}

void CUIController::PlayMonsterHPDeleteCreate(CHandle pHandle)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);
	_float scaleRatio = pBtn->GetScaleRatio();
	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(scaleRatio, 0.5f, 0.2f,
		[pBtn](float currentValue) {
			pBtn->SetScaleRatio(currentValue);
			pBtn->CalcUICoord();
		}, nullptr, EEaseType::EaseOutQuad);

	pTween->PlayTween(Alpah, 0.f, 0.2f,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
			pBtn->CalcUICoord();
		}, [pHandle, this]() {
			GET_SINGLE(UIManager)->DeleteUIRecursive(pHandle);
			m_bMonsterHP = true;
			}, EEaseType::EaseOutQuad);
}

void CUIController::PlayDividerUPWidth(CHandle pHandle)
{
	CUIObject* pUI = SafeGetOBJ(pHandle);
	auto pTween = pUI->GetTweenCom();

	//pUI->SetInputLcok(true);

	_float width = pUI->GetSize().x;
	_float height = pUI->GetSize().y;
	_float Alpah = pUI->GetAlpha();

	pTween->PlayTween(256.f, width + 256.f, 1.f,
		[pUI, height](float currentValue) {
			pUI->SetSize({ currentValue, height });
			pUI->CalcUICoord();
		}, nullptr, EEaseType::Linear, 3.f);

	pTween->PlayTween(0.f, 1.f, 1.f,
		[pUI](float currentValue) {
			pUI->SetAlpha(currentValue);
			pUI->CalcUICoord();
		}, nullptr, EEaseType::Linear, 3.f);

}

void CUIController::PlayAlphaUP(CHandle pHandle, float delaytime, float playTime)
{
	CUIObject* pUI = SafeGetOBJ(pHandle);
	auto pTween = pUI->GetTweenCom();

	pUI->SetInputLcok(true);

	_float Alpah = pUI->GetAlpha();

	pTween->PlayTween(0.f, 1.f, playTime,
		[pUI](float currentValue) {
			pUI->SetAlpha(currentValue);
			pUI->CalcUICoord();
		}, nullptr, EEaseType::Linear, delaytime);
}

void CUIController::Free()
{
	if (m_iQuestUIListenerID != 0)
	{
		E::CGameInstance::Get().
			EventUnsubscribe<FQuestUIGroupChanged>(m_iQuestUIListenerID);
		m_iQuestUIListenerID = 0;
	}

	m_hMiniMap = std::nullopt;
	CGameObject::Free();
}

E::UPtr<CUIController> CUIController::Create()
{
	auto pInstance = E::ToUPtr(new CUIController{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CUIController");
		return nullptr;
	}
	return  pInstance;
}


E::UPtr<E::CPrototype> CUIController::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CUIController{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CUIController");
		return nullptr;
	}

	return pInstance;
}
