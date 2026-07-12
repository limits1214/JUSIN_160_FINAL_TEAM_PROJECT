#include "pch.h"
#include "LevelUIEditor.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "Client_Defines.h"
#include "FlyCamera.h"
#include "UiCamera.h"
#include "ResCBuffer.h"
#include "ResTexture2D.h"
#include "CTexUI.h"
#include "UIObject.h"
#include "FlipBook.h"
#include <fstream>
#include "TextureUI.h"
#include "TextBox.h"
#include "TextUI.h"
#include "FlipbookUI.h"
#include "TextUI.h"
#include "EffectUI.h"
#include "UIManager.h"


namespace fs = std::filesystem;

NS_USING(Client)

static int selectedParent = -1;

CLevelUIEditor::CLevelUIEditor()
{
}

CLevelUIEditor::~CLevelUIEditor()
{
}

HRESULT CLevelUIEditor::Initialize()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	E::CGameInstance::Get().GameObjectAllReset();

	Target_UI = std::nullopt;
	m_iEditorMode = 0;
	m_iButtonMode = 0;
	count = 0;

	m_vResTag.push_back("TEX_SHM");
	m_vResTag.push_back("TEX_MAP");
	m_vResTag.push_back("TEX_UI_T_NurtureMeterDiamond_Back_4k");
	m_vResTag.push_back("TEX_UI_T_NurtureMeterDiamond_Ready_4k");
	m_vResTag.push_back("TEX_UI_T_NurtureMeterDiamond_Outer_4k");

	m_vFlipBookResTag.push_back("Flipbook_LoadingWidget_Flame");
	m_vFlipBookResTag.push_back("Flipbook_LoadingWidget_Houses");
	m_vFlipBookResTag.push_back("Flipbook_VFXSmokeSim_D");
	m_vFlipBookResTag.push_back("Flipbook_VFX_T_ItemSpark_8x8_D");
	m_vFlipBookResTag.push_back("Flipbook_VFX_T_PopVFX_8x8_D");
	m_vFlipBookResTag.push_back("Flipbook_VFX_BlinkingStars");
	m_vFlipBookResTag.push_back("Flipbook_UI_T_MagicEffect1");
	m_vFlipBookResTag.push_back("Flipbook_UI_T_SmokeWispy_D");

	if (std::nullopt == Target_UI)
	{
		m_UIINFO.fX		= clientSize.x * 0.5f;
		m_UIINFO.fY = clientSize.y * 0.5f;
		m_UIINFO.SizeX = 100.f;
		m_UIINFO.SizeY = 100.f;
		m_UIINFO.Alpha = 1.f;
		m_UIINFO.Weight	= 0;
		m_UIINFO.Name = "Name";
		strcpy_s(m_cName, "Name");
	}

	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
		Desc.vAt = { 0.f, 0.f, 0.f };
		Desc.vEye = { 0.f, 0.f, -5.f };
		Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
		Desc.fFovY = 75.f;
		Desc.fNear = 0.1f;
		Desc.fFar = 100.f;
		Desc.sObjectTag = "FlyCam";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_FlyCamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("FLY", flyCam.value())))
			{
				int x = 0;
			}
			E::CGameInstance::Get().SetActiveCamera("FLY");
		}
	}

	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::ORTHOGRAPHIC;
		Desc.fNear = 0.f;
		Desc.fFar = 1.f;
		Desc.fWidth = g_iWinSizeX;
		Desc.fHeight = g_iWinSizeY;
		Desc.sObjectTag = "UICam";
		Desc.vEye = { 0.f, 0.f, -0.1f };

		if (auto uiCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_UICamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("UI", uiCam.value())))
			{
				int x = 0;
			}

			// dynamic_cast vs static_cast benchmark 
			// dynamic_cast: 472ms, static_cast: 41ms
			if constexpr (false)
			{
				E::CGameObject* volatile val = E::CGameInstance::Get().GetGameObjectByHandle(uiCam.value());
				{
					auto start = std::chrono::high_resolution_clock::now();
					{
						E::CUICamera* volatile sink = nullptr;
						for (size_t i = 0; i < 10'000'000; ++i)
						{
							sink = dynamic_cast<E::CUICamera*>(val);
						}
					}
					auto end = std::chrono::high_resolution_clock::now();
					auto cost = std::chrono::duration<double, std::milli>(end - start).count();
					MSG_BOX_STR(std::to_wstring(cost).c_str());
				}
				{
					auto start = std::chrono::high_resolution_clock::now();
					{
						E::CUICamera* volatile sink = nullptr;
						for (size_t i = 0; i < 10'000'000; ++i)
						{
							if (val->IsA(E::CUICamera::StaticType))
							{
								sink = static_cast<E::CUICamera*>(val);
							}
							else
							{
								sink = nullptr;
							}
						}
					}
					auto end = std::chrono::high_resolution_clock::now();
					auto cost = std::chrono::duration<double, std::milli>(end - start).count();
					MSG_BOX_STR(std::to_wstring(cost).c_str());
				}
			}
			{
				const auto* t = E::CGameInstance::GetConst().GetGameObjectByHandleT<E::CUICamera>(uiCam.value());

				auto* t2 = E::CGameInstance::Get().GetGameObjectByHandleT<E::CUICamera>(uiCam.value());
			}
		}
	}

	{
		if (std::nullopt == Target_UI)
		{
			m_UIINFO.fX = clientSize.x * 0.5f;
			m_UIINFO.fY = clientSize.y * 0.5f;
			m_UIINFO.SizeX = 100.f;
			m_UIINFO.SizeY = 100.f;
			m_UIINFO.Alpha = 1.f;
			m_UIINFO.Weight = 0;
			m_UIINFO.Name = "None";

			strcpy_s(m_cName, "Name");
		}
	}

	GET_SINGLE(UIManager)->InitializeActions();

	return S_OK;
}

void CLevelUIEditor::Update(E::_float fTimeDelta)
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	_bool bP = CGameInstance::Get().KeyDown(DIK_P);
	_bool bLShift = CGameInstance::Get().KeyPressing(DIK_LSHIFT);
	_bool bDelete = CGameInstance::Get().KeyDown(DIK_DELETE);

	_bool bCreate = false;
	if (std::nullopt != m_oSelectHandle)
		bCreate = true;

	//const _tchar* text = L"Test";
	//CGameInstance::Get().FontAddLateDraw(RENDERGROUP::UI, "NeoDGM_15px", text, { clientSize.x * 0.5f, clientSize.y * 0.5f });

	if (bP)
	{
		// debug용 
		if(false)
		{
			count++;
			CUIObject::UIOBJECT_DESC Desc{};
			Desc.sObjectTag = "UI_" + std::to_string(count);
			Desc.fSizeX = 200.f;
			Desc.fSizeY = 200.f;
			Desc.fX = clientSize.x * 0.5f;
			Desc.fY = clientSize.y * 0.5f;
			Desc.fAlpha = 1.f;
			Desc.ResTag = "TEX_MAP";
			Desc.ResWeight = count;
			Desc.UIType = ETOUI(UI_TYPE::TEXUI);

			std::optional<CHandle> handle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		}

		if (false)
		{
			count++;
			CUIObject::UIOBJECT_DESC Desc{};
			Desc.sObjectTag = "UI_" + std::to_string(count);
			Desc.fSizeX = 200.f;
			Desc.fSizeY = 200.f;
			Desc.fX = clientSize.x * 0.5f;
			Desc.fY = clientSize.y * 0.5f;
			Desc.fAlpha = 1.f;
			Desc.ResTag = "TEX_MAP";
			Desc.ResWeight = count;
			Desc.UIType = ETOUI(UI_TYPE::TEXUI);

			std::optional<CHandle> handle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		}

		if (true)
		{
			count++;
			CTextUI::TEXT_DESC desc{};

			desc.fSizeX = 2.f;
			desc.fSizeY = 2.f;
			desc.fX = clientSize.x * 0.5f;
			desc.fY = clientSize.y * 0.5f;
			desc.fAlpha = 0.05f;
			desc.Text = L"Test";

			std::optional<CHandle> handle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextBox", "Layer_UI", &desc);
		}
	}
	if (bLShift)
	{
		if (std::nullopt != m_oSelectHandle)
		{
			Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*m_oSelectHandle);
			selectUI->SetPendingDestroyCascade();

			m_oSelectHandle = std::nullopt;
		}
		m_iButtonMode = ETOUI(UiButtonMode::SELECT);
	}
	else if (bCreate)
	{
		Target_UI = std::nullopt;
		m_iButtonMode = ETOUI(UiButtonMode::CREATE);
	}
	else
		m_iButtonMode = ETOUI(UiButtonMode::DEFAULT);

	// Default
	if (bDelete)
	{
		if (std::nullopt != m_oSelectHandle)
		{
			Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*m_oSelectHandle);
			selectUI->SetPendingDestroyCascade();

			m_oSelectHandle = std::nullopt;
		}

		if (std::nullopt != Target_UI)
		{
			DeleteUIRecursive(Target_UI);

			Target_UI = std::nullopt;
		}
	}

	if (std::nullopt != m_oSelectHandle)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*m_oSelectHandle);

		if (selectUI == nullptr)
			return;

		UI_INFO& selectInfo = selectUI->GetUIInfo();
		selectInfo.SizeX = m_UIINFO.SizeX;
		selectInfo.SizeY = m_UIINFO.SizeY;
		selectUI->CalcUICoord();
	}

	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		UI_INFO& selectInfo = selectUI->GetUIInfo();

		selectInfo.fX = m_UIINFO.fX;
		selectInfo.fY = m_UIINFO.fY;
		selectInfo.SizeX = m_UIINFO.SizeX;
		selectInfo.SizeY = m_UIINFO.SizeY;
		selectInfo.Alpha = m_UIINFO.Alpha;
		selectInfo.Weight = m_UIINFO.Weight;
		selectInfo.Name = m_cName;
		selectInfo.Color = m_UIINFO.Color;
		selectInfo.UIType = m_UIINFO.UIType;
		selectInfo.EffectType = m_UIINFO.EffectType;

		if (ETOUI(UI_TYPE::FLIPBOOK) == *selectUI->GetUIType())
		{
			FLIP_INFO& flipInfo = static_cast<CFlipbookUI*>(selectUI)->GetFlipInfo();

			m_FLIPINFO.cellsize = flipInfo.cellsize;
			m_FLIPINFO.Duration = flipInfo.Duration;
			m_FLIPINFO.TotalFrame = flipInfo.TotalFrame;
			m_FLIPINFO.Padding = flipInfo.Padding;
		}
		selectUI->CalcUICoord();
	}

	switch (m_iButtonMode)
	{
	case ETOUI(UiButtonMode::DEFAULT):
		break;
	case ETOUI(UiButtonMode::CREATE):
		CreateMode();
		break;
	case ETOUI(UiButtonMode::SELECT):
		SelectMode();
		break;
	default:
		break;
	}
}

HRESULT CLevelUIEditor::Render()
{
	return S_OK;
}

void CLevelUIEditor::UpdateGUI()
{

	switch (m_iEditorMode)
	{
	case ETOUI(UiEditorMode::ARRANGE):
		ArrangeMode();
		break;
	case ETOUI(UiEditorMode::PREFAB):
		PrefabMode();
		break;
	case ETOUI(UiEditorMode::FLIPBOOK):
		FlipbookMode();
		break;
	default:
		break;
	}
}

void CLevelUIEditor::FrameStart(E::_float fTimeDelta)
{
}

void CLevelUIEditor::CreateMode()
{
	_bool MouseLB = CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB);

	if (std::nullopt != m_oSelectHandle)
	{
		if (MouseLB)
		{
			CTextureUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*m_oSelectHandle);
			const UI_INFO& selectInfo = selectUI->GetUIInfo();
			_float2 mousePos = E::CGameInstance::Get().GetMousePos();

			CTextureUI::UIOBJECT_DESC Desc{};

			count++;
			Desc.sObjectTag = "UI_" + std::to_string(count);
			Desc.Name = "UI_" + std::to_string(count);
			Desc.fSizeX = selectInfo.SizeX;
			Desc.fSizeY = selectInfo.SizeY;
			Desc.fX = mousePos.x;
			Desc.fY = mousePos.y;
			Desc.fAlpha = 1.f;
			Desc.ResTag = selectInfo.Restag;
			Desc.UIType = ETOUI(UI_TYPE::TEXUI);
			Desc.ResWeight = count;

			E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		}
	}
}

void CLevelUIEditor::SelectMode()
{
	_bool MouseLB = CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB);
	_bool MouseLBPressing = CGameInstance::Get().MousePressing(MOUSEKEYSTATE::LB);

	if (MouseLB)
	{
		if (m_iEditorMode == ETOUI(UiEditorMode::ARRANGE))
			PickingOnlyRoot();
		else
			Picking();
	}
		

	if (MouseLBPressing && std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		UI_INFO& selectInfo = selectUI->GetUIInfo();

		_float2 mousePos = CGameInstance::Get().GetMousePos();

		m_UIINFO.fX = mousePos.x - m_vDragOffset.x;
		m_UIINFO.fY = mousePos.y - m_vDragOffset.y;

		if (std::nullopt != selectUI->GetParent())
		{
			Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*selectUI->GetParent());
			UI_INFO& parentInfo = parentUI->GetUIInfo();

			selectInfo.LocalX = m_UIINFO.fX - parentInfo.fX;
			selectInfo.LocalY = m_UIINFO.fY - parentInfo.fY;
		}
		else
		{
			selectInfo.fX = m_UIINFO.fX;
			selectInfo.fY = m_UIINFO.fY;
		}
		selectUI->CalcUICoord();
	}
}

void CLevelUIEditor::ArrangeMode()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	
	_float2 mousePos = CGameInstance::Get().GetMousePos();

	DrawJsonFileLoader(m_iEditorMode);

	ImGui::Begin("EDITOR_MODE: ARRANGE_MODE");

	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Save / Load");
	ImGui::Separator();

	if (ImGui::Button("Save"))
		Save();

	ImGui::SameLine();

	if (ImGui::Button("Load"))
		Load();

	ImGui::SetNextItemWidth(100);
	ImGui::InputText("LevelName", m_cLevelName, sizeof(m_cLevelName));

	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Select_Mode");
	ImGui::Separator();

	if (ImGui::Button("ARRAGE_MODE"))
	{
		m_iEditorMode = ETOUI(UiEditorMode::ARRANGE);
		RefreshJsonFileList();
	}
	ImGui::SameLine();
	if (ImGui::Button("PREFAB_MODE"))
	{
		m_iEditorMode = ETOUI(UiEditorMode::PREFAB);
		RefreshJsonFileList();
	}
	ImGui::SameLine();
	if (ImGui::Button("FLIPBOOK_MODE"))
	{
		m_iEditorMode = ETOUI(UiEditorMode::FLIPBOOK);
		RefreshJsonFileList();
	}

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Target_State");
	ImGui::Separator();

	ImGui::Text("PosX  : %.2f  ", m_UIINFO.fX);
	ImGui::SameLine(150);
	ImGui::Text("PosY   : %.2f", m_UIINFO.fY);

	ImGui::Text("SizeX : %.2f  ", m_UIINFO.SizeX);
	ImGui::SameLine(150);
	ImGui::Text("SizeY  : %.2f", m_UIINFO.SizeY);

	ImGui::Text("Alpha : %.2f", m_UIINFO.Alpha);
	ImGui::SameLine(150);
	ImGui::Text("Weight : %.d", m_UIINFO.Weight);

	ImGui::Text("Name : %s", m_cName);

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Input_State");
	ImGui::Separator();

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fx", &m_UIINFO.fX, 0.1f, 0.0f, clientSize.x);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fy", &m_UIINFO.fY, 0.1f, 0.0f, clientSize.y);

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fSizeX", &m_UIINFO.SizeX, 0.1f, 0.0f, clientSize.x);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fSizeY", &m_UIINFO.SizeY, 0.1f, 0.0f, clientSize.y);

	ImGui::SetNextItemWidth(80);
	ImGui::DragFloat("fAlpha", &m_UIINFO.Alpha, 0.001f, 0.0f, 1.f);
	ImGui::SameLine(150);
	ImGui::SetNextItemWidth(80);
	ImGui::DragInt("iWeight", &m_UIINFO.Weight, 1.f, 0.0f, 100);

	ImGui::SetNextItemWidth(80);
	ImGui::InputText("Name", m_cName, sizeof(m_cName));

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Select_Level");
	ImGui::Separator();

	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "Select_Images");
	ImGui::Separator();

	if (ImGui::BeginTable("TextureTable", 2))
	{
		for (size_t i = 0; i < m_vResTag.size(); ++i)
		{
			ImGui::TableNextColumn();

			ImGui::PushID((int)i);

			const auto& srv = E::CGameInstance::GetConst()
				.GetResourceFirst<E::CResTexture2D>("LEVEL_UIEDITOR", m_vResTag[i]);

			if (ImGui::ImageButton((ImTextureID)srv->GetSRV().Get(), ImVec2(100, 100)))
			{
				if (std::nullopt != m_oSelectHandle)
				{
					CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_oSelectHandle);
					selectUI->SetPendingDestroyCascade();

					m_oSelectHandle = std::nullopt;
				}

				CTextureUI::UIOBJECT_DESC Desc{};

				Desc.sObjectTag = "Select_Image";
				Desc.fSizeX = m_UIINFO.SizeX;
				Desc.fSizeY = m_UIINFO.SizeY;
				Desc.fX = g_iWinSizeX * 0.5f;
				Desc.fY = g_iWinSizeY * 0.5f;
				Desc.fAlpha = m_UIINFO.Alpha * 0.3f;
				Desc.ResTag = m_vResTag[i];
				Desc.UIType = ETOUI(UI_TYPE::TEXUI);
				Desc.ResWeight = 10000;

				m_oSelectHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI","Layer_UI_Texture", &Desc);
				CTextureUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*m_oSelectHandle);
				selectUI->SetMouseTracking(true);
			}

			ImGui::PopID();
		}

		ImGui::EndTable();
	}

	ImGui::End();
}  

void CLevelUIEditor::PrefabMode()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	_float2 mousePos = CGameInstance::Get().GetMousePos();
	
	DrawJsonFileLoader(m_iEditorMode);
	
	// 윈도우 여백 및 패딩 약간 조절
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
	ImGui::Begin("UI Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	
	// ---------------------------------------------------------
	// 1. File & Mode Settings
	// ---------------------------------------------------------
	if (ImGui::CollapsingHeader("File & Mode Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Prefab Name:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(200);
		ImGui::InputText("##PrefabName", m_cPrefabName, sizeof(m_cPrefabName));
	
		if (ImGui::Button("Save Prefab", ImVec2(120, 0)))
			PrefabSave();
	
		ImGui::SameLine();
	
		if (ImGui::Button("Load Prefab", ImVec2(120, 0)))
			GET_SINGLE(UIManager)->LoadPrefab(m_cPrefabName, g_PrefabPath);
	
		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[ Editor Mode ]");
	
		// 라디오 버튼 형태나 그룹화된 버튼으로 모드 전환을 직관적으로 변경
		if (ImGui::Button("ARRANGE", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::ARRANGE);
			RefreshJsonFileList();
		}
		ImGui::SameLine();
		if (ImGui::Button("PREFAB", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::PREFAB);
			RefreshJsonFileList();
		}
		ImGui::SameLine();
		if (ImGui::Button("FLIPBOOK", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::FLIPBOOK);
			RefreshJsonFileList();
		}
	}
	
	// ---------------------------------------------------------
	// 2. Hierarchy / Parent Setting
	// ---------------------------------------------------------
	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* targetUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		m_UIINFO.Name = targetUI->GetName();
		strcpy_s(m_cName, sizeof(m_cName), m_UIINFO.Name.c_str());
	
		if (nullptr != CGameInstance::Get().GetGameObjectLayer("Layer_UI"))
		{
			if (ImGui::CollapsingHeader("Hierarchy Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				std::vector<Engine::CUIObject*> uiList;
				std::vector<CHandle> uiHandles = *CGameInstance::Get().GetGameObjectLayer("Layer_UI");
	
				for (auto ui : uiHandles)
				{
					Engine::CUIObject* checkUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(ui);
					if (checkUI && ui != Target_UI) // 자기 자신 제외
						uiList.push_back(checkUI);
				}
	
				const char* preview = (selectedParent >= 0 && selectedParent < uiList.size()) ? uiList[selectedParent]->GetName() : "None";
	
				ImGui::SetNextItemWidth(250);
				if (ImGui::BeginCombo("Parent UI", preview))
				{
					if (ImGui::Selectable("None", selectedParent == -1))
					{
						selectedParent = -1;
						targetUI->SetParent(std::nullopt);
					}
	
					for (int i = 0; i < uiList.size(); ++i)
					{
						bool isSelected = (selectedParent == i);
						if (ImGui::Selectable(uiList[i]->GetName(), isSelected))
						{
							selectedParent = i;
							targetUI->SetParent(uiList[i]->GetHandle());
	
							Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(uiList[i]->GetHandle());
							parentUI->AddChildren(*Target_UI);
	
							UI_INFO& targetInfo = targetUI->GetUIInfo();
							UI_INFO& parentInfo = parentUI->GetUIInfo();
	
							targetInfo.LocalX = m_UIINFO.fX - parentInfo.fX;
							targetInfo.LocalY = m_UIINFO.fY - parentInfo.fY;
							targetUI->CalcUICoord();
						}
						if (isSelected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndCombo();
				}
			}
		}
	
		// ---------------------------------------------------------
		// 3. Properties
		// ---------------------------------------------------------
		if (std::nullopt != targetUI->GetParent())
			LocalStateView();
		else
			StateView();
	}
	else
	{
		StateView();
	}
	
	// ---------------------------------------------------------
	// 4. Resource / Image Selector
	// ---------------------------------------------------------
	if (ImGui::CollapsingHeader("Texture Resources"))
	{
		// 가로 패딩을 15로 증가 (기존은 기본값 8)
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 10));

		// 스크롤 가능한 차일드 영역 부여
		ImGui::BeginChild("TextureView", ImVec2(0, 300), true);
		if (ImGui::BeginTable("TextureTable", 4)) // 컬럼 수를 늘려서 한 줄에 여러 이미지 배치
		{
			for (size_t i = 0; i < m_vResTag.size(); ++i)
			{
				ImGui::TableNextColumn();
				ImGui::PushID((int)i);
	
				const auto& srv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>("LEVEL_UIEDITOR", m_vResTag[i]);
	
				// 텍스처 툴팁 기능 추가 (Hover 시 크게 보기 등 가능)
				if (ImGui::ImageButton((ImTextureID)srv->GetSRV().Get(), ImVec2(60, 60)))
				{
					if (std::nullopt != m_oSelectHandle)
					{
						CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_oSelectHandle);
						selectUI->SetPendingDestroyCascade();
						m_oSelectHandle = std::nullopt;
					}
	
					CTextureUI::UIOBJECT_DESC Desc{};
					Desc.sObjectTag = "Select_Image";
					Desc.fSizeX = m_UIINFO.SizeX;
					Desc.fSizeY = m_UIINFO.SizeY;
					Desc.fX = g_iWinSizeX * 0.5f;
					Desc.fY = g_iWinSizeY * 0.5f;
					Desc.fAlpha = m_UIINFO.Alpha * 0.3f;
					Desc.ResTag = m_vResTag[i];
					Desc.UIType = ETOUI(UI_TYPE::TEXUI);
					Desc.ResWeight = 10000;
	
					m_oSelectHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI", "Layer_UI_Texture", &Desc);
					CTextureUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*m_oSelectHandle);
					selectUI->SetMouseTracking(true);
				}

				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();

					// 텍스처 이름(태그) 표시 (선택 사항)
					ImGui::Text("ResTag: %s", m_vResTag[i].c_str()); 

					// 크게 보여줄 이미지 사이즈 지정 (예: 256x256)
					ImGui::Image((ImTextureID)srv->GetSRV().Get(), ImVec2(256, 256));

					ImGui::EndTooltip();
				}

				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		ImGui::EndChild();

		ImGui::PopStyleVar();
	}
	
	ImGui::End();
	ImGui::PopStyleVar();
}

void CLevelUIEditor::FlipbookMode()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	_float2 mousePos = CGameInstance::Get().GetMousePos();
	
	DrawJsonFileLoader(m_iEditorMode);
	
	// 전체 윈도우 패딩 적용
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
	ImGui::Begin("Flipbook Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	
	// ---------------------------------------------------------
	// 1. File & Mode Settings
	// ---------------------------------------------------------
	if (ImGui::CollapsingHeader("File & Mode Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[ Editor Mode ]");
	
		if (ImGui::Button("ARRANGE", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::ARRANGE);
			RefreshJsonFileList();
		}
		ImGui::SameLine();
		if (ImGui::Button("PREFAB", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::PREFAB);
			RefreshJsonFileList();
		}
		ImGui::SameLine();
		if (ImGui::Button("FLIPBOOK", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::FLIPBOOK);
			RefreshJsonFileList();
		}
	
		ImGui::Spacing();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Flipbook Name:");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(200);
		ImGui::InputText("##FlipbookName", m_cPrefabName, sizeof(m_cPrefabName));
	
		if (ImGui::Button("Make Flipbook", ImVec2(120, 0)))
			FlipBookMake();
		ImGui::SameLine();
		if (ImGui::Button("Save Flipbook", ImVec2(120, 0)))
			PrefabSave();
	}
	
	// ---------------------------------------------------------
	// 2. Animation Properties
	// ---------------------------------------------------------
	if (ImGui::CollapsingHeader("Animation Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::BeginTable("AnimSettingsTable", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable))
		{
			ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 100.0f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
			ImGui::Text("ResTag"); ImGui::TableNextColumn();
			// m_UIINFO.Restag가 할당되지 않았을 경우를 대비해 처리
			if (m_UIINFO.Restag.empty())
				ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "None");
			else
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", m_UIINFO.Restag.c_str());
	
			int cellsize = m_FLIPINFO.cellsize;
			int TotalFrame = m_FLIPINFO.TotalFrame;
			int Padding = m_FLIPINFO.Padding;

			ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
			ImGui::Text("Cell Size"); ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(100);
			ImGui::InputInt("##CellSize", &cellsize);
			m_FLIPINFO.cellsize = cellsize;
	
			ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
			ImGui::Text("Total Frame"); ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(100);
			ImGui::InputInt("##TotalFrame", &TotalFrame);
			m_FLIPINFO.TotalFrame = TotalFrame;
	
			ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
			ImGui::Text("Padding"); ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(100);
			ImGui::InputInt("##Padding", &Padding);
			m_FLIPINFO.Padding = Padding;
	
			ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
			ImGui::Text("Duration"); ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(100);
			ImGui::InputFloat("##Duration", &m_FLIPINFO.Duration);
	
			ImGui::EndTable();
		}
	}
	
	// ---------------------------------------------------------
	// 3. Target State Update & View
	// ---------------------------------------------------------
	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
	
		if (selectUI != nullptr && (ETOUI(UI_TYPE::FLIPBOOK) == *selectUI->GetUIType()))
		{
			// 실시간으로 입력값 동기화
			FLIP_INFO& flipInfo = static_cast<CFlipbookUI*>(selectUI)->GetFlipInfo();
			flipInfo.cellsize = m_FLIPINFO.cellsize;
			flipInfo.TotalFrame = m_FLIPINFO.TotalFrame;
			flipInfo.Padding = m_FLIPINFO.Padding;
			flipInfo.Duration = m_FLIPINFO.Duration;
	
			if (std::nullopt != selectUI->GetParent())
				LocalStateView();
			else
				StateView();
		}
		else
		{
			StateView();
		}
	}
	else
	{
		StateView();
	}
	
	// ---------------------------------------------------------
	// 4. Texture Resources
	// ---------------------------------------------------------
	if (ImGui::CollapsingHeader("Texture Resources", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 잘림 방지용 내부 패딩
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 10));
	
		// 스크롤 가능한 차일드 영역
		ImGui::BeginChild("TextureView", ImVec2(0, 300), true);
	
		// 4개의 열(Column)로 이미지 나열
		if (ImGui::BeginTable("TextureTable", 4))
		{
			for (size_t i = 0; i < m_vFlipBookResTag.size(); ++i)
			{
				ImGui::TableNextColumn();
				ImGui::PushID((int)i);
	
				const auto& srv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>("LEVEL_UIEDITOR", m_vFlipBookResTag[i]);
	
				if (ImGui::ImageButton((ImTextureID)srv->GetSRV().Get(), ImVec2(60, 60)))
				{
					m_UIINFO.Restag = m_vFlipBookResTag[i].c_str();
				}
	
				// 호버 시 크게 보기 툴팁 추가
				if (ImGui::IsItemHovered())
				{
					ImGui::BeginTooltip();
					ImGui::Text("Tag: %s", m_vFlipBookResTag[i].c_str());
					ImGui::Image((ImTextureID)srv->GetSRV().Get(), ImVec2(256, 256));
					ImGui::EndTooltip();
				}
	
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		ImGui::EndChild();
	
		ImGui::PopStyleVar(); // 패딩 스타일 복원
	}
	
	ImGui::End();
	ImGui::PopStyleVar(); // 메인 윈도우 스타일 복원
}

void CLevelUIEditor::AnimationMode()
{
	ImGui::Begin("UI Animation Tool");
	
	// ... 클립 이름 입력 및 JSON 로드/저장 로직 ...
	
	if (ImGui::Button("Add Track"))
	{
		//m_CurrentClip.Tracks.push_back(FUITweenTrack());
	}
	
	for (int i = 0; i < m_CurrentClip.Tracks.size(); ++i)
	{
		ImGui::PushID(i);
		auto& track = m_CurrentClip.Tracks[i];
	
		const char* targetNames[] = { "SCALE", "EFFECT_ALPHA", "POSITION_X", "POSITION_Y" };
		ImGui::Combo("Target", (int*)&track.TargetType, targetNames, IM_ARRAYSIZE(targetNames));
	
		ImGui::Checkbox("Use Current Start", &track.bUseCurrentStart);
		if (!track.bUseCurrentStart)
			ImGui::DragFloat("Start Value", &track.fStartValue, 0.01f);
	
		ImGui::DragFloat("End Value", &track.fEndValue, 0.01f);
		ImGui::DragFloat("Duration", &track.fDuration, 0.01f);
	
		ImGui::PopID();
	}
	ImGui::End();
}

void CLevelUIEditor::Picking()
{
	_float2 mousePos = CGameInstance::Get().GetMousePos();

	std::vector<CHandle> uiHandles = *CGameInstance::Get().GetGameObjectLayer("Layer_UI");

	Target_UI = std::nullopt;

	uint32_t maxWeight = 0;

	for (auto ui : uiHandles)
	{
		Engine::CUIObject* checkUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(ui);

		if (nullptr == checkUI)
			continue;

		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(ui);
		const UI_INFO& selectInfo = selectUI->GetUIInfo();

		_float2 origin = { selectInfo.fX, selectInfo.fY };
		_float2 size = { selectInfo.SizeX, selectInfo.SizeY };

		_float2 minPos =
		{
			origin.x - size.x * 0.5f,
			origin.y - size.y * 0.5f
		};

		_float2 maxPos =
		{
			origin.x + size.x * 0.5f,
			origin.y + size.y * 0.5f
		};

		if (mousePos.x >= minPos.x &&
			mousePos.x <= maxPos.x &&
			mousePos.y >= minPos.y &&
			mousePos.y <= maxPos.y)
		{
			uint32_t curWeight = selectInfo.Weight;
			if (curWeight >= maxWeight)
			{
				maxWeight = curWeight;
				Target_UI = ui;

				m_vDragOffset = { CGameInstance::Get().GetMousePos().x - origin.x,
					CGameInstance::Get().GetMousePos().y - origin.y };
			}
		}
	}

	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		UI_INFO& selectInfo = selectUI->GetUIInfo();
		m_UIINFO.fX = selectInfo.fX;
		m_UIINFO.fY = selectInfo.fY;
		m_UIINFO.SizeX = selectInfo.SizeX;
		m_UIINFO.SizeY = selectInfo.SizeY;
		m_UIINFO.Alpha = selectInfo.Alpha;
		m_UIINFO.Weight = selectInfo.Weight;
		m_UIINFO.Color = selectInfo.Color;
		m_UIINFO.UIType = selectInfo.UIType;
		m_UIINFO.EffectType = selectInfo.EffectType;
		strcpy_s(m_cName, sizeof(m_cName), selectInfo.Name.c_str());

		if (ETOUI(UI_TYPE::FLIPBOOK) == selectInfo.UIType)
		{
			FLIP_INFO& flipInfo = static_cast<CFlipbookUI*>(selectUI)->GetFlipInfo();

			m_FLIPINFO.cellsize = flipInfo.cellsize;
			m_FLIPINFO.Duration = flipInfo.Duration;
			m_FLIPINFO.TotalFrame = flipInfo.TotalFrame;
			m_FLIPINFO.Padding = flipInfo.Padding;
		}
	}
}

void CLevelUIEditor::PickingOnlyRoot()
{
	_float2 mousePos = CGameInstance::Get().GetMousePos();

	if (nullptr == CGameInstance::Get().GetGameObjectLayer("Layer_UI"))
		return;

	std::vector<CHandle> uiHandles = *CGameInstance::Get().GetGameObjectLayer("Layer_UI");

	Target_UI = std::nullopt;

	uint32_t maxWeight = 0;

	for (auto ui : uiHandles)
	{
		Engine::CUIObject* checkUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(ui);

		if (nullptr == checkUI)
			continue;

		if (std::nullopt != checkUI->GetParent())
			continue;

		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(ui);
		const UI_INFO& selectInfo = selectUI->GetUIInfo();

		_float2 origin = { selectInfo.fX, selectInfo.fY };
		_float2 size = { selectInfo.SizeX, selectInfo.SizeY};

		_float2 minPos =
		{
			origin.x - size.x * 0.5f,
			origin.y - size.y * 0.5f
		};

		_float2 maxPos =
		{
			origin.x + size.x * 0.5f,
			origin.y + size.y * 0.5f
		};

		if (mousePos.x >= minPos.x &&
			mousePos.x <= maxPos.x &&
			mousePos.y >= minPos.y &&
			mousePos.y <= maxPos.y)
		{
			uint32_t curWeight = selectInfo.Weight;
			if (curWeight >= maxWeight)
			{
				maxWeight = curWeight;
				Target_UI = ui;

				m_vDragOffset = { CGameInstance::Get().GetMousePos().x - origin.x,
					CGameInstance::Get().GetMousePos().y - origin.y };
			}
		}
	}

	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		UI_INFO& selectInfo = selectUI->GetUIInfo();
		m_UIINFO.fX = selectInfo.fX;
		m_UIINFO.fY = selectInfo.fY;
		m_UIINFO.SizeX = selectInfo.SizeX;
		m_UIINFO.SizeY = selectInfo.SizeY;
		m_UIINFO.Alpha = selectInfo.Alpha;
		m_UIINFO.Weight = selectInfo.Weight;
		strcpy_s(m_cName, sizeof(m_cName), selectInfo.Name.c_str());

		if (ETOUI(UI_TYPE::FLIPBOOK) == selectInfo.UIType)
		{
			FLIP_INFO& flipInfo = static_cast<CFlipbookUI*>(selectUI)->GetFlipInfo();

			m_FLIPINFO.cellsize = flipInfo.cellsize;
			m_FLIPINFO.Duration = flipInfo.Duration;
			m_FLIPINFO.TotalFrame = flipInfo.TotalFrame;
			m_FLIPINFO.Padding = flipInfo.Padding;
		}
	}
}

void CLevelUIEditor::Save()
{
	std::vector<CHandle> uiHandles = *CGameInstance::Get().GetGameObjectLayer("Layer_UI");

	nlohmann::ordered_json root;

	root["LevelName"] = m_cLevelName;
	root["UI"] = nlohmann::ordered_json::array();

	for (CHandle handle : uiHandles)
	{
		CTexUI* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTexUI>(handle);

		if (pUI == nullptr)
			continue;

		if (pUI->GetParent().has_value())
			continue;

		nlohmann::ordered_json obj;
		SaveUIRecursive(pUI, obj);

		root["UI"].push_back(obj);
	}

	char path[256] = "./Resources/SampleClient/UIData/LevelUI/";
	strcat_s(path, sizeof(path), m_cLevelName);
	char final[256] = ".json";
	strcat_s(path, sizeof(path), final);

	std::ofstream file(path);
	if (!file.is_open())
	{
		MSG_BOX("파일 저장 실패");
		return;
	}
	else
	{
		MSG_BOX("파일 저장 성공");
	}
	file << root.dump(4);

	file.close();
}

void CLevelUIEditor::Load()
{
	char path[256] = "./Resources/SampleClient/UIData/LevelUI/";
	strcat_s(path, sizeof(path), m_cLevelName);
	strcat_s(path, sizeof(path), ".json");

	std::ifstream file(path);

	if (!file.is_open())
	{
		MSG_BOX("파일 열기 실패");
		return;
	}

	nlohmann::ordered_json root;
	file >> root;
	file.close();

	for (const auto& obj : root["UI"])
	{
		LoadUIRecursive(obj, nullptr);
	}

}

void CLevelUIEditor::PrefabSave()
{
	std::vector<CHandle> uiHandles = *CGameInstance::Get().GetGameObjectLayer("Layer_UI");

	nlohmann::ordered_json root;

	root["PrefabName"] = m_cPrefabName;
	root["UI"] = nlohmann::ordered_json::array();

	for (CHandle handle : uiHandles)
	{
		E::CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<E::CUIObject>(handle);

		if (pUI == nullptr)
			continue;

		if (pUI->GetParent().has_value())
			continue;

		nlohmann::ordered_json obj;
		SaveUIRecursive(pUI, obj);

		root["UI"].push_back(obj);
	}

	switch (m_iEditorMode)
	{
	case ETOUI(UiEditorMode::ARRANGE):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/LevelUI/");
		break;
	case ETOUI(UiEditorMode::PREFAB):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/Prefabs/");
		break;
	case ETOUI(UiEditorMode::FLIPBOOK):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/FlipBook/");
		break;
	}
	
	char path[256] = "";
	strcpy_s(path, sizeof(path), g_BasePath);
	strcat_s(path, sizeof(path), m_cPrefabName);
	char final[256] = ".json";
	strcat_s(path, sizeof(path), final);

	std::ofstream file(path);
	if (!file.is_open())
	{
		MSG_BOX("파일 저장 실패");
		return;
	}
	else
	{
		MSG_BOX("파일 저장 성공");
	}
	file << root.dump(4);

	file.close();
}

void CLevelUIEditor::PrefabLoad()
{
	switch (m_iEditorMode)
	{
	case ETOUI(UiEditorMode::ARRANGE):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/LevelUI/");
		break;
	case ETOUI(UiEditorMode::PREFAB):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/Prefabs/");
		break;
	case ETOUI(UiEditorMode::FLIPBOOK):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/FlipBook/");
		break;
	}

	char path[256] = "";
	strcpy_s(path, sizeof(path), g_BasePath);
	strcat_s(path, sizeof(path), m_cPrefabName);
	strcat_s(path, sizeof(path), ".json");

	std::ifstream file(path);

	if (!file.is_open())
	{
		MSG_BOX("파일 열기 실패");
		return;
	}

	nlohmann::ordered_json root;
	file >> root;
	file.close();

	for (const auto& obj : root["UI"])
	{
		LoadUIRecursive(obj, nullptr);
	}
}

void CLevelUIEditor::FlipBookMake()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	count++;
	CFlipbookUI::FLIPBOOK_DESC Desc{};
	Desc.sObjectTag = "UI_" + std::to_string(count);
	Desc.fSizeX = 200.f;
	Desc.fSizeY = 200.f;
	Desc.fX = clientSize.x * 0.5f;
	Desc.fY = clientSize.y * 0.5f;
	Desc.fAlpha = 1.f;
	Desc.ResTag = m_UIINFO.Restag;
	Desc.ResWeight = count;
	Desc.UIType = ETOUI(UI_TYPE::FLIPBOOK);

	std::optional<CHandle> handle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_EffectUI", "Layer_UI", &Desc);
	CEffectUI* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CEffectUI>(*handle);

	FLIP_INFO& flipInfo = pUI->GetFlipInfo();
	flipInfo.cellsize = m_FLIPINFO.cellsize;
	flipInfo.TotalFrame = m_FLIPINFO.TotalFrame;
	flipInfo.Padding = m_FLIPINFO.Padding;
	flipInfo.Duration = m_FLIPINFO.Duration;

	
}

void CLevelUIEditor::SaveUIRecursive(E::CUIObject* pUI, nlohmann::ordered_json& obj)
{
	UI_INFO& uiInfo = pUI->GetUIInfo();

	obj["UiType"] = uiInfo.UIType;
	obj["UI_EFFECT_TYPE"] = uiInfo.EffectType;

	obj["Name"] = uiInfo.Name;

	obj["X"] = uiInfo.fX;
	obj["Y"] = uiInfo.fY;

	obj["LocalX"] = uiInfo.LocalX;
	obj["LocalY"] = uiInfo.LocalY;

	obj["SizeX"] = uiInfo.SizeX;
	obj["SizeY"] = uiInfo.SizeY;

	obj["WidthRatioX"] = uiInfo.WidthRatioX;
	obj["WidthRatioY"] = uiInfo.WidthRatioY;
	
	obj["Rot"] = uiInfo.Rot;
	obj["LocalRot"] = uiInfo.LocalRot;

	obj["Alpha"] = uiInfo.Alpha;
	obj["AlphaRatio"] = uiInfo.AlphaRatio;

	obj["Weight"] = uiInfo.Weight;
	obj["WeightOffset"] = uiInfo.WeightOffset;

	obj["ResTag"] = uiInfo.Restag;

	obj["Color"] = { uiInfo.Color.x, uiInfo.Color.y, uiInfo.Color.z };

	switch (uiInfo.UIType)
	{
	case ETOUI(UI_TYPE::TEXUI):
		break;
	case ETOUI(UI_TYPE::FLIPBOOK):
	{
		const FLIP_INFO& flipInfo = static_cast<CFlipbookUI*>(pUI)->GetFlipInfo();
		obj["CellSize"] = flipInfo.cellsize;
		obj["TotalFrame"] = flipInfo.TotalFrame;
		obj["Padding"] = flipInfo.Padding;
		obj["Duration"] = flipInfo.Duration;
		break;
	}
	case ETOUI(UI_TYPE::TEXT):
	{
		const TEXT_INFO& textInfo = static_cast<CTextUI*>(pUI)->GetTextInfo();
		//obj["Text"] = textInfo.Text;
	}
	default:
		break;
	}

	obj["Children"] = nlohmann::ordered_json::array();

	for (CHandle childHandle : pUI->GetChildren())
	{
		E::CUIObject* pChild = E::CGameInstance::Get().GetGameObjectByHandleT<E::CUIObject>(childHandle);

		if (pChild == nullptr)
			continue;

		nlohmann::ordered_json childObj;
		SaveUIRecursive(pChild, childObj);

		obj["Children"].push_back(childObj);
	}
}

E::CUIObject* CLevelUIEditor::LoadUIRecursive(const nlohmann::ordered_json& obj, E::CUIObject* parent)
{
	int uiType = obj["UiType"];
	count++;
	E::CUIObject* pUI = nullptr;

	E::CUIObject::UIOBJECT_DESC Desc{};
	std::optional<CHandle> uiHandle = std::nullopt;

	Desc.sObjectTag = obj["Name"];

	switch (uiType)
	{
	case ETOUI(UI_TYPE::TEXUI):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::FLIPBOOK):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_EffectUI", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CEffectUI>(*uiHandle);
		{
			FLIP_INFO& flipInfo = static_cast<CEffectUI*>(pUI)->GetFlipInfo();
			flipInfo.cellsize	= obj["CellSize"];
			flipInfo.TotalFrame = obj["TotalFrame"];
			flipInfo.Padding	= obj["Padding"];
			flipInfo.Duration	= obj["Duration"];
		}
		break;
	case ETOUI(UI_TYPE::TEXT):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextBox", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(*uiHandle);
		{
			TEXT_INFO& textInfo = static_cast<CTextBox*>(pUI)->GetTextInfo();
			//textInfo.Text = obj["Text"];
		}
		
	default:
		break;
	}

	UI_INFO& uiInfo = static_cast<CUIObject*>(pUI)->GetUIInfo();

	if (pUI == nullptr)
		return nullptr;

	uiInfo.EffectType = obj["UI_EFFECT_TYPE"];
	uiInfo.Name = obj["Name"];
	
	uiInfo.SizeX = obj["SizeX"];
	uiInfo.SizeY = obj["SizeY"];
	
	uiInfo.Alpha = obj["Alpha"];
	uiInfo.AlphaRatio = obj["AlphaRatio"];
	
	uiInfo.Weight = obj["Weight"];
	uiInfo.WeightOffset = obj["WeightOffset"];
	
	uiInfo.LocalX = obj["LocalX"];
	uiInfo.LocalY = obj["LocalY"];
	
	uiInfo.WidthRatioX = obj["WidthRatioX"];
	uiInfo.WidthRatioY = obj["WidthRatioY"];
	
	uiInfo.Restag = obj["ResTag"];
	
	uiInfo.Rot = obj["Rot"];
	uiInfo.LocalRot = obj["LocalRot"];
	
	auto color = obj["Color"];
	uiInfo.Color = { color[0], color[1], color[2] };

	if (parent == nullptr)
	{
		uiInfo.fX = obj["X"];
		uiInfo.fY = obj["Y"];
	}
	else
	{
		pUI->SetParent(parent->GetHandle());
		parent->AddChildren(pUI->GetHandle());

		uiInfo.LocalX = obj["LocalX"];
		uiInfo.LocalY = obj["LocalY"];
	}

	// 부모 기준으로 다시 계산
	pUI->CalcUICoord();

	for (const auto& child : obj["Children"])
	{
		LoadUIRecursive(child, pUI);
	}

	return pUI;
}

void CLevelUIEditor::StateView()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	
	if (!ImGui::CollapsingHeader("Global Properties", ImGuiTreeNodeFlags_DefaultOpen))
	return;
	
	// 부모 노드 이름 처리
	if (std::nullopt != Target_UI)
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		std::optional<CHandle> parentNode = selectUI->GetParent();
		if (parentNode != std::nullopt)
		{
			Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*parentNode);
			strcpy_s(m_sParentName, parentUI->GetName());
		}
		else strcpy_s(m_sParentName, "None");
	}
	else strcpy_s(m_sParentName, "None");
	
	
	// 2개의 컬럼을 가진 프로퍼티 테이블 생성
	if (ImGui::BeginTable("GlobalStateTable", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
	
		// Name
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Name"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##Name", m_cName, sizeof(m_cName));
		m_UIINFO.Name = m_cName;
	
		// Parent (Read Only)
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Parent"); ImGui::TableNextColumn();
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), m_sParentName);
	
		// Transform (Position)
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Position"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("X##PosX", &m_UIINFO.fX, 0.1f, 0.0f, clientSize.x); ImGui::SameLine();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("Y##PosY", &m_UIINFO.fY, 0.1f, 0.0f, clientSize.y);
	
		// Transform (Size)
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Size"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("W##SizeX", &m_UIINFO.SizeX, 0.1f, 0.0f, clientSize.x); ImGui::SameLine();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("H##SizeY", &m_UIINFO.SizeY, 0.1f, 0.0f, clientSize.y);
	
		// Alpha & Weight
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Alpha"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("##Alpha", &m_UIINFO.Alpha, 0.001f, 0.0f, 1.f);
	
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Weight"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragInt("##Weight", &m_UIINFO.Weight, 1, 0, 100);
	
		// Color
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Color (RGB)"); ImGui::TableNextColumn();
		float colorArr[3] = { m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z };
		ImGui::SetNextItemWidth(200);
		if (ImGui::ColorEdit3("##Color", colorArr))
		{
			m_UIINFO.Color.x = colorArr[0];
			m_UIINFO.Color.y = colorArr[1];
			m_UIINFO.Color.z = colorArr[2];
		}
	
		// Enums (UI Type & Effect)
		static const char* UITypeNames[] = { "CONTAINER", "TEXUI", "FLIPBOOK", "TEXT", "BUTTON" };
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("UI Type"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(150);
		ImGui::Combo("##UIType", reinterpret_cast<int*>(&m_UIINFO.UIType), UITypeNames, IM_ARRAYSIZE(UITypeNames));
	
		static const char* EffectTypeNames[] = { "NONE", "HOVER", "CLICK" };
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Effect Type"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(150);
		ImGui::Combo("##EffectType", reinterpret_cast<int*>(&m_UIINFO.EffectType), EffectTypeNames, IM_ARRAYSIZE(EffectTypeNames));
	
		ImGui::EndTable();
	}
}

void CLevelUIEditor::LocalStateView()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
	std::optional<CHandle> parentNode = selectUI->GetParent();
	Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*parentNode);

	UI_INFO& selectInfo = selectUI->GetUIInfo();

	// 참조 대신 값 복사를 통한 수정용 변수 (원본과 연결됨)
	_float localX = selectInfo.LocalX;
	_float localY = selectInfo.LocalY;
	_float widthX = selectInfo.WidthRatioX;
	_float widthY = selectInfo.WidthRatioY;
	_float alphaRatio = selectInfo.AlphaRatio;
	int weightOffset = selectInfo.WeightOffset;

	if (!ImGui::CollapsingHeader("Local Properties (Child)", ImGuiTreeNodeFlags_DefaultOpen))
	return;

	if (ImGui::BeginTable("LocalStateTable", 2, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_Resizable))
	{
		ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 100.0f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

		// Name
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Name"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputText("##LocalName", m_cName, sizeof(m_cName));
		m_UIINFO.Name = m_cName;

		// Parent
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Parent"); ImGui::TableNextColumn();
		ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), parentUI->GetName());

		// Local Transform
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Local Pos"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("X##LPosX", &localX, 0.1f, -clientSize.x, clientSize.x); ImGui::SameLine();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("Y##LPosY", &localY, 0.1f, -clientSize.y, clientSize.y);

		// Width Ratio
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Width Ratio"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("X##WRatioX", &widthX, 0.001f, 0.0f, 5.f); ImGui::SameLine();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("Y##WRatioY", &widthY, 0.001f, 0.0f, 5.f);

		// Alpha & Weight
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Alpha Ratio"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("##AlphaRatio", &alphaRatio, 0.001f, 0.0f, 1.f);

		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Weight Offset"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragInt("##WeightOffset", &weightOffset, 1, -100, 100);

		// Color 
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Color (RGB)"); ImGui::TableNextColumn();
		float colorArr[3] = { m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z };
		ImGui::SetNextItemWidth(200);
		if (ImGui::ColorEdit3("##LocalColor", colorArr))
		{
			m_UIINFO.Color.x = colorArr[0];
			m_UIINFO.Color.y = colorArr[1];
			m_UIINFO.Color.z = colorArr[2];
		}

		// Enums
		static const char* UITypeNames[] = { "CONTAINER", "TEXUI", "FLIPBOOK", "TEXT", "BUTTON" };
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("UI Type"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(150);
		ImGui::Combo("##LUIType", reinterpret_cast<int*>(&m_UIINFO.UIType), UITypeNames, IM_ARRAYSIZE(UITypeNames));

		static const char* EffectTypeNames[] = { "NONE", "HOVER", "CLICK" };
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Effect Type"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(150);
		ImGui::Combo("##LEffectType", reinterpret_cast<int*>(&m_UIINFO.EffectType), EffectTypeNames, IM_ARRAYSIZE(EffectTypeNames));

		ImGui::EndTable();
	}

	// Update Target Properties
	selectInfo.LocalX = localX;
	selectInfo.LocalY = localY;
	selectInfo.WidthRatioX = widthX;
	selectInfo.WidthRatioY = widthY;
	selectInfo.AlphaRatio = alphaRatio;
	selectInfo.WeightOffset = weightOffset;
}

void CLevelUIEditor::DrawFileExplorer()
{

}

void CLevelUIEditor::DeleteUIRecursive(std::optional<CHandle> targetHandle)
{
	Engine::CUIObject* targetUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*targetHandle);

	std::vector<CHandle>  childHandles = targetUI->GetChildren();

	for (auto childHandle : childHandles)
	{
		DeleteUIRecursive(childHandle);
	}

	if (targetUI->GetParent())
	{
		Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*targetUI->GetParent());

		if (nullptr != parentUI)
			parentUI->DeleteChild(targetUI->GetHandle());
	}

	selectedParent--;
	targetUI->SetPendingDestroyCascade();

	return;
}

void CLevelUIEditor::RefreshJsonFileList()
{
	g_JsonFiles.clear();

	switch (m_iEditorMode)
	{
	case ETOUI(UiEditorMode::ARRANGE) :
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/LevelUI/");
		break;
	case ETOUI(UiEditorMode::PREFAB):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/Prefabs/");
		break;
	case ETOUI(UiEditorMode::FLIPBOOK):
		strcpy_s(g_BasePath, "./Resources/SampleClient/UIData/FlipBook/");
		break;
	}

	if (!fs::exists(g_BasePath) || !fs::is_directory(g_BasePath))
		return;

	try
	{
		for (const auto& entry : fs::directory_iterator(g_BasePath))
		{
			// 오직 일반 파일이면서 확장자가 .json인 것만 수집
			if (entry.is_regular_file() && entry.path().extension() == ".json")
			{
				JsonFileInfo info;
				info.fileName = entry.path().filename().string();
				info.fullPath = entry.path().string();
				g_JsonFiles.push_back(info);
			}
		}
	}
	catch (const std::exception& e)
	{
		// 에러 처리 예시
	}
}

void CLevelUIEditor::DrawJsonFileLoader(uint32_t EditorMode)
{
	if (!g_IsFileGridInitialized)
	{
		RefreshJsonFileList();
		g_IsFileGridInitialized = true;
	}
	std::string title = "";
	switch (EditorMode)
	{
	case ETOUI(UiEditorMode::ARRANGE):
		title = "Level Selector";
		break;
	case ETOUI(UiEditorMode::PREFAB):
		title = "PREFAB Selector";
		break;
	case ETOUI(UiEditorMode::FLIPBOOK):
		title = "FLIPBOOK Selector";
		break;
	}

	ImGui::Begin(title.c_str());

	// 파일이 추가되었을 때를 대비한 새로고침 버튼
	if (ImGui::Button("Refresh"))
	{
		RefreshJsonFileList();
	}

	ImGui::Separator();

	// 스크롤 가능한 목록 영역 시작
	ImGui::BeginChild("JsonListArea", ImVec2(0, 0), true);

	if (g_JsonFiles.empty())
	{
		ImGui::TextDisabled("경로 내에 JSON 파일이 없습니다.");
	}
	else
	{
		for (const auto& file : g_JsonFiles)
		{
			// 리스트 형태로 파일명을 출력하고, 클릭 감지
			if (ImGui::Selectable(file.fileName.c_str(), false))
			{
				switch (EditorMode)
				{
				case ETOUI(UiEditorMode::ARRANGE):
					strcpy_s(m_cLevelName, sizeof(m_cLevelName), file.fileName.substr(0, file.fileName.length() - 5).c_str());
					Load();
					break;
				case ETOUI(UiEditorMode::PREFAB):
					strcpy_s(m_cPrefabName, sizeof(m_cPrefabName), file.fileName.substr(0, file.fileName.length() - 5).c_str());
					GET_SINGLE(UIManager)->LoadPrefab(m_cPrefabName, g_PrefabPath);
					break;
				case ETOUI(UiEditorMode::FLIPBOOK):
					strcpy_s(m_cPrefabName, sizeof(m_cPrefabName), file.fileName.substr(0, file.fileName.length() - 5).c_str());
					GET_SINGLE(UIManager)->LoadPrefab(m_cPrefabName, g_FlipbookPath);
					break;
				}

				// 디버깅용 콘솔 출력
				printf("로드 대상 파일: %s\n", file.fullPath.c_str());
			}
		}
	}

	ImGui::EndChild();
	ImGui::End();
}

Engine::UPtr<CLevelUIEditor> CLevelUIEditor::Create()
{
	auto	pInstance = Engine::UPtr<CLevelUIEditor>(new CLevelUIEditor{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevelUIEditor");
	}

	return pInstance;
}

void CLevelUIEditor::Free()
{
	E::CGameInstance::Get().DelPrototype("LEVEL_UIEditor");
	E::CGameInstance::Get().DelResource("LEVEL_UIEditor");
	CLevel::Free();
}
