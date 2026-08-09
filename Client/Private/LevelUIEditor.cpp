#include "pch.h"
#include "LevelUIEditor.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "Level_Defines.h"
#include "Client_Defines.h"
#include "FlyCamera.h"
#include "UiCamera.h"
#include "ResCBuffer.h"
#include "ResTexture2D.h"
#include "UIObject.h"
#include <fstream>
#include "TextureUI.h"
#include "TextBox.h"
#include "TextUI.h"
#include "FlipbookUI.h"
#include "EffectUI.h"
#include "UIManager.h"


namespace fs = std::filesystem;

NS_USING(Client)

static int selectedParent = -1;

CLevelUIEditor::CLevelUIEditor()
	: CLevel{ ETOUI(LEVEL::UIEDITOR) }
{
}

CLevelUIEditor::~CLevelUIEditor()
{
}

HRESULT CLevelUIEditor::Initialize()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	E::CGameInstance::Get().GameObjectAllResetExceptLayers({
		"00_ENGINE_CINEMATIC_CAMERA"
	});

	GET_SINGLE(UIManager)->CreateFadeOut();

	Target_UI = std::nullopt;
	m_iEditorMode = 1;
	m_iButtonMode = 0;
	count = 0;

	CGameInstance::Get().GetGameObjectLayer("Layer_UI");

	m_vResTag.push_back("TEX_SHM");
	m_vResTag.push_back("TEX_MAP");

	// SY가 수정함
	const auto pResourceMap = E::CGameInstance::Get().GetResource("LEVEL_UIEDITOR");
	m_vResTag.clear();
	for (const auto& pair : pResourceMap)
	{
		m_vResTag.push_back(pair.first.GetDbgStr());
	}

	m_vFlipBookResTag.push_back("Flipbook_LoadingWidget_Flame");
	m_vFlipBookResTag.push_back("Flipbook_LoadingWidget_Houses");
	m_vFlipBookResTag.push_back("Flipbook_VFXSmokeSim_D");
	m_vFlipBookResTag.push_back("Flipbook_VFX_T_ItemSpark_8x8_D");
	m_vFlipBookResTag.push_back("Flipbook_VFX_T_PopVFX_8x8_D");
	m_vFlipBookResTag.push_back("Flipbook_VFX_BlinkingStars");
	m_vFlipBookResTag.push_back("Flipbook_UI_T_MagicEffect1");
	m_vFlipBookResTag.push_back("Flipbook_UI_T_SmokeWispy_D");
	m_vFlipBookResTag.push_back("TEX_VFX_T_ImpactDust_FB_D"); 
	m_vFlipBookResTag.push_back("TEX_VFX_T_TMB_SmokeWispy_D");
	m_vFlipBookResTag.push_back("TEX_VFX_T_Fireball_Dir_01_D");
	m_vFlipBookResTag.push_back("TEX_VFX_T_FireballB_01_D");
	m_vFlipBookResTag.push_back("TEX_VFX_T_Fireball_Stream_D");
	
	if (std::nullopt == Target_UI)
	{
		m_UIINFO.fX	= clientSize.x * 0.5f;
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

	return S_OK;
}

void CLevelUIEditor::Update(E::_float fTimeDelta)
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	_bool bF1 = CGameInstance::Get().KeyDown(DIK_F1);
	_bool bF2 = CGameInstance::Get().KeyDown(DIK_F2);
	_bool bLShift = CGameInstance::Get().KeyPressing(DIK_LSHIFT);
	_bool bDelete = CGameInstance::Get().KeyDown(DIK_DELETE);

	_bool bCreate = false;
	if (std::nullopt != m_oSelectHandle)
		bCreate = true;

	//const _tchar* text = L"Test";
	//CGameInstance::Get().FontAddLateDraw(RENDERGROUP::UI, "NeoDGM_15px", text, { clientSize.x * 0.5f, clientSize.y * 0.5f });

	if (bF1)
	{
		if (true)
		{
			CTextureUI::UIOBJECT_DESC Desc{};

			count++;
			Desc.sObjectTag = "UI_" + std::to_string(count);
			Desc.Name = "UI_" + std::to_string(count);
			Desc.fSizeX = 1280.f;
			Desc.fSizeY = 720.f;
			Desc.fX = clientSize.x * 0.5f;
			Desc.fY = clientSize.y * 0.5f;
			Desc.fAlpha = 1.f;
			Desc.UIType = ETOUI(UI_TYPE::VIDEOOBJ);
			Desc.ResWeight = count;

			E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_VideoObject", "Layer_UI", &Desc);
		}
		// 붉은 구름
		if (false)
		{
			CTextureUI::UIOBJECT_DESC Desc{};

			count++;
			Desc.sObjectTag = "UI_" + std::to_string(count);
			Desc.Name = "UI_" + std::to_string(count);
			Desc.fSizeX = 512.f;
			Desc.fSizeY = 128.f;
			Desc.fX = clientSize.x * 0.5f;
			Desc.fY = clientSize.y * 0.5f;
			Desc.fAlpha = 1.f;
			Desc.UIType = ETOUI(UI_TYPE::GAMEOVERMASK);
			Desc.ResWeight = count;

			E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_GameOverMask", "Layer_UI", &Desc);
		}
		if (false)
		{
			count++;
			CTextUI::TEXT_DESC desc{};

			desc.sObjectTag = "UI_" + std::to_string(count);
			desc.Name = "UI_" + std::to_string(count);
			desc.fSizeX = 3.f;
			desc.fSizeY = 3.f;
			desc.fX = clientSize.x * 0.5f;
			desc.fY = clientSize.y * 0.5f;
			desc.fAlpha = 1.f;
			desc.Text = L"Test";
			desc.ResWeight = count;

			std::optional<CHandle> handle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextBox", "Layer_UI", &desc);
		}
	}

	if (bF2)
	{
		if (true)
		{
			count++;
			CTextUI::TEXT_DESC desc{};

			desc.sObjectTag = "UI_" + std::to_string(count);
			desc.Name = "";
			desc.fSizeX = 3.f;
			desc.fSizeY = 3.f;
			desc.fX = clientSize.x * 0.5f;
			desc.fY = clientSize.y * 0.5f;
			desc.fAlpha = 1.f;
			desc.Text = L"Test";
			desc.ResWeight = count;

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

	UpdateTargetState();
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

	GET_SINGLE(UIManager)->UpdateRootUIHandles();

	
}

HRESULT CLevelUIEditor::Render()
{
	return S_OK;
}

void CLevelUIEditor::UpdateGUI()
{
	//ResetProperty(Target_UI);
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
	//UpdateTargetState();
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
		if (ImGui::Button("PREFAB", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::PREFAB);
			RefreshJsonFileList();
		}
		ImGui::SameLine();
		if (ImGui::Button("FLIPBOOK", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::FLIPBOOK);
			RefreshJsonFileList();
		}
		ImGui::SameLine();
		if (ImGui::Button("Text", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::ARRANGE);
			RefreshJsonFileList();
		}

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[ Clear ]");
		if (ImGui::Button("ClearUI", ImVec2(90, 0))) {
			ClearUI();
		}
	}

	if (std::nullopt != Target_UI &&
		(nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI)))
	{
		Engine::CUIObject* targetUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		m_UIINFO.Name = targetUI->GetName();
		strcpy_s(m_cName, sizeof(m_cName), m_UIINFO.Name.c_str());
	}

	StateView();

	if (ImGui::CollapsingHeader("Global UI Hierarchy", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 스크롤이 가능하도록 영역 지정 (UI가 많아질 것을 대비)
		ImGui::BeginChild("HierarchyTreeBox", ImVec2(0, 200), true);

		std::vector<CHandle> rootUIHandles = GET_SINGLE(UIManager)->GetRootUIHandles();

		for (auto rootHandle : rootUIHandles)
		{
			DrawHierarchyNode(rootHandle); // 여기서부터 재귀적으로 쭉 그려짐
		}

		// 빈 공간을 클릭하면 선택 해제 (원한다면 추가)
		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered())
		{
			Target_UI = std::nullopt;
		}

		ImGui::EndChild();
	}

	ImGui::End();
	ImGui::PopStyleVar();
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
		if (ImGui::Button("PREFAB", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::PREFAB);
			RefreshJsonFileList();
		}
		ImGui::SameLine();
		if (ImGui::Button("FLIPBOOK", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::FLIPBOOK);
			RefreshJsonFileList();
		}
		ImGui::SameLine();
		if (ImGui::Button("Text", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::ARRANGE);
			RefreshJsonFileList();
		}

		ImGui::Spacing();
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[ Clear ]");
		if (ImGui::Button("ClearUI", ImVec2(90, 0))) {
			ClearUI();
		}

		ImGui::Separator();
		ImGui::Text("--- 3D World Space Settings ---");

		// 1. 3D 변환 토글 버튼
		if (ImGui::Checkbox("Is World Space UI", &m_IsWorldSpace))
		{
			// 체크박스 상태가 변할 때 현재 선택된 UI 객체에 즉시 반영
			if (Target_UI != std::nullopt &&
				nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*Target_UI))
			{
				CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*Target_UI);
				m_WorldPos = { 0.f, 0.f, 0.f };
				pUI->GetTransform().SetPosition(m_WorldPos);
				ApplyWorldSpaceRecursive(pUI, m_IsWorldSpace, m_fWorldScaleFactor);
			}
		}

		if (m_IsWorldSpace)
		{
			ImGui::Indent();

			// 스케일 팩터 조절 
			if (ImGui::DragFloat("World Scale Factor", &m_fWorldScaleFactor, 0.001f, 0.0001f, 1.0f, "%.4f"))
			{
				if (Target_UI != std::nullopt &&
					nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*Target_UI))
				{
					CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*Target_UI);
					//pUI->SetWorldScaleFactor(m_fWorldScaleFactor);
				}
			}

			if (ImGui::DragFloat3("World Position (X,Y,Z)", (float*)&m_WorldPos, 0.1f))
			{
				if (Target_UI != std::nullopt &&
					nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*Target_UI))
				{
					CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*Target_UI);
					pUI->GetTransform().SetPosition(XMLoadFloat3(&m_WorldPos));
				}
			}

			// 타겟 기준 오프셋 조절
			//if (ImGui::DragFloat3("World Offset (X,Y,Z)", (float*)&m_vWorldOffset, 0.1f))
			//{
			//	if (pSelectedUI) pSelectedUI->SetWorldOffset(m_vWorldOffset);
			//}
			//
			//// 빌보드 (카메라 마주보기) 토글
			//if (ImGui::Checkbox("Look At Camera (Billboard)", &m_bIsBillboard))
			//{
			//	if (pSelectedUI) pSelectedUI->SetBillboard(m_bIsBillboard);
			//}
			//
			//// 뎁스 무시 (투시) 토글
			//if (ImGui::Checkbox("Ignore Depth (Render on Top)", &m_bIgnoreDepth))
			//{
			//	if (pSelectedUI) pSelectedUI->SetIgnoreDepth(m_bIgnoreDepth);
			//}

			ImGui::Unindent(); // 들여쓰기 복구
		}
		ImGui::Separator();
	}
	
	// ---------------------------------------------------------
	// 2. Hierarchy / Parent Setting
	// ---------------------------------------------------------
	if (std::nullopt != Target_UI && 
		(nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI)))
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
	// 4. Event & Action Settings (새로 추가된 구역)
	// ---------------------------------------------------------
	if (ImGui::CollapsingHeader("Event & Action Settings", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 검색과 스크롤이 지원되는 고급 콤보박스 람다
		auto DrawEventCombo = [](const char* label, std::string& current_item, const std::vector<std::string>* items) {
			ImGui::SetNextItemWidth(200.0f);

			// ImGuiComboFlags_HeightLarge를 주어 팝업이 너무 작게 열리는 것을 방지
			if (ImGui::BeginCombo(label, current_item.empty() ? "None" : current_item.c_str(), ImGuiComboFlags_HeightLarge))
			{
				// ImGui 내장 텍스트 필터 (콤보 팝업은 한 번에 하나만 열리므로 static 공유가 안전함)
				static ImGuiTextFilter filter;

				// 콤보박스가 처음 열릴 때 필터를 초기화하고, 즉시 타이핑할 수 있게 포커스를 줌
				if (ImGui::IsWindowAppearing()) {
					filter.Clear();
					ImGui::SetKeyboardFocusHere();
				}

				// 상단 고정 검색창 (클릭하지 않아도 바로 타이핑 가능)
				filter.Draw("##Search", ImGui::GetContentRegionAvail().x);
				ImGui::Separator();

				// None 항목은 검색어와 무관하게 항상 최상단에 고정
				if (ImGui::Selectable("None", current_item.empty())) {
					current_item = "";
					ImGui::CloseCurrentPopup();
				}

				if (items) {
					// 검색 결과가 많을 때 검색창은 상단에 고정하고 리스트만 스크롤되도록 Child 영역 생성
					ImGui::BeginChild("##ComboList", ImVec2(0, 200), false);
					for (const auto& item : *items) {

						// 필터(검색어)에 맞지 않는 문자열은 렌더링 건너뛰기
						if (!filter.PassFilter(item.c_str()))
							continue;

						bool is_selected = (current_item == item);
						if (ImGui::Selectable(item.c_str(), is_selected)) {
							current_item = item;
							ImGui::CloseCurrentPopup(); // 항목 선택 시 팝업 닫기
						}

						// 방향키로 탐색할 때 현재 선택된 항목에 포커스
						if (is_selected) ImGui::SetItemDefaultFocus();
					}
					ImGui::EndChild();
				}
				ImGui::EndCombo();
			}
		};

		// 변경할 이벤트 구조체의 포인터를 가져옵니다.
		// 타겟 UI가 있으면 타겟 UI의 이벤트를, 없으면 에디터 자신이 들고 있는 이벤트를 수정합니다.
		UI_EVENT* pTargetEvent = &m_UIEVENT;
		if (Target_UI.has_value()) {
			Engine::CUIObject* pTargetObj = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
			if (pTargetObj) {
				// [참고] CUIObject 안에 m_UIEVENT가 public이거나, 
				// UI_EVENT& GetUIEvent() { return m_UIEVENT; } 처럼 참조형 반환 함수가 있어야 합니다.
				pTargetEvent = &pTargetObj->GetUIEvent();
			}
		}

		// UIManager에서 벡터 포인터 가져오기
		std::vector<std::string>* pFuncNames = GET_SINGLE(UIManager)->GetFuncNames();
		std::vector<std::string>* pEventNames = GET_SINGLE(UIManager)->GetEventNames();

		// 기능 함수 1개 (m_vFuncNames)
		DrawEventCombo("Click Function", pTargetEvent->ClickFunc, pFuncNames);

		ImGui::Spacing();

		// 이펙트 액션 5개 (m_vEventNames)
		DrawEventCombo("Click Action", pTargetEvent->ClickAction, pEventNames);
		DrawEventCombo("Enter Action", pTargetEvent->EnterAction, pEventNames);
		DrawEventCombo("Exit Action", pTargetEvent->ExitAction, pEventNames);
		DrawEventCombo("Appear Action", pTargetEvent->AppearAction, pEventNames);
		DrawEventCombo("Disappear Action", pTargetEvent->DisappearAction, pEventNames);
	}


	// ---------------------------------------------------------
	// 4. Event & Action Settings (하이라키)
	// ---------------------------------------------------------
	if (ImGui::CollapsingHeader("Global UI Hierarchy", ImGuiTreeNodeFlags_DefaultOpen))
	{
		// 스크롤이 가능하도록 영역 지정 (UI가 많아질 것을 대비)
		ImGui::BeginChild("HierarchyTreeBox", ImVec2(0, 200), true);

		std::vector<CHandle> rootUIHandles = GET_SINGLE(UIManager)->GetRootUIHandles();

		for (auto rootHandle : rootUIHandles)
		{
			DrawHierarchyNode(rootHandle); // 여기서부터 재귀적으로 쭉 그려짐
		}

		// 빈 공간을 클릭하면 선택 해제 (원한다면 추가)
		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered())
		{
			Target_UI = std::nullopt;
		}

		ImGui::EndChild();
	}
	
	// ---------------------------------------------------------
	// 5. Resource / Image Selector
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
					const D3D11_TEXTURE2D_DESC& texDesc = srv->GetTexture2DDesc();

					CTextureUI::UIOBJECT_DESC Desc{};
					Desc.sObjectTag = "Select_Image";

					Desc.fSizeX = static_cast<float>(texDesc.Width);
					Desc.fSizeY = static_cast<float>(texDesc.Height);

					Desc.fX = g_iWinSizeX * 0.5f;
					Desc.fY = g_iWinSizeY * 0.5f;
					Desc.fAlpha = m_UIINFO.Alpha * 0.3f;
					Desc.ResTag = m_vResTag[i];
					Desc.UIType = ETOUI(UI_TYPE::TEXUI);
					Desc.ResWeight = 10000;

					m_oSelectHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI", "Layer_UI_Texture", &Desc);
					CTextureUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*m_oSelectHandle);
					selectUI->SetMouseTracking(true);

					// (선택 사항) 만약 방금 생성한 UI의 크기 데이터를 에디터 프로퍼티 창에도 
					// 바로 갱신해서 띄워주고 싶다면 아래 코드를 추가하세요.
					 m_UIINFO.SizeX = static_cast<float>(texDesc.Width);
					 m_UIINFO.SizeY = static_cast<float>(texDesc.Height);
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

		
		if (ImGui::Button("PREFAB", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::PREFAB);
			RefreshJsonFileList();
		}
		ImGui::SameLine();
		if (ImGui::Button("FLIPBOOK", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::FLIPBOOK);
			RefreshJsonFileList();
		}
		ImGui::SameLine();
		if (ImGui::Button("Text", ImVec2(90, 0))) {
			m_iEditorMode = ETOUI(UiEditorMode::ARRANGE);
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

	Target_UI = GET_SINGLE(UIManager)->RootUIPicking();

	if (Target_UI == std::nullopt)
	{
		m_IsWorldSpace = false;
		return;
	}

	Engine::CUIObject* ptargetUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
	UI_INFO& ptargetInfo = ptargetUI->GetUIInfo();
	m_vDragOffset = { CGameInstance::Get().GetMousePos().x - ptargetInfo.fX,
		CGameInstance::Get().GetMousePos().y - ptargetInfo.fY };

	if (std::nullopt != Target_UI && 
		nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI))
	{
		ResetProperty(Target_UI);
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
		E::CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<E::CUIObject>(handle);

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

	obj["ScaleRatio"] = pUI->GetScaleRatio();
	obj["LocalScaleRatio"] = pUI->GetLocalScaleRatio();

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

	obj["ClickFunc"] = pUI->GetUIEvent().ClickFunc;
	obj["ClickAction"] = pUI->GetUIEvent().ClickAction;
	obj["EnterAction"] = pUI->GetUIEvent().EnterAction;
	obj["ExitAction"] = pUI->GetUIEvent().ExitAction;
	obj["AppearAction"] = pUI->GetUIEvent().AppearAction;
	obj["DisappearAction"] = pUI->GetUIEvent().DisappearAction;

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
		obj["Text"] = WStringToUTF8(textInfo.Text);
	}
	default:
		break;
	}

	obj["IsWorldSpace"] = pUI->GetWorldSpace();
	if (pUI->GetWorldSpace())
	{
		_float3 pos = pUI->GetTransform().GetPosition();
		obj["WorldPos"] = { pos.x, pos.y, pos.z };
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

	if (pUI == nullptr)
		return nullptr;

	UI_INFO& uiInfo = static_cast<CUIObject*>(pUI)->GetUIInfo();

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

	UI_EVENT& eventInfo = pUI->GetUIEvent();

	eventInfo.ClickFunc = obj.value("ClickFunc", "");
	eventInfo.ClickAction = obj.value("ClickAction", "");
	eventInfo.EnterAction = obj.value("EnterAction", "");
	eventInfo.ExitAction = obj.value("ExitAction", "");
	eventInfo.AppearAction = obj.value("AppearAction", "");
	eventInfo.DisappearAction = obj.value("DisappearAction", "");

	// [추가] 로드한 문자열을 바탕으로 실제 함수(콜백)를 UI 객체에 매핑
	// (단, "None"이거나 비어있으면 매핑하지 않음)
	auto bindAction = [](const std::string& actionStr, std::function<void(CUIObject*)>& targetFunc) {
		if (!actionStr.empty() && actionStr != "None") {
			targetFunc = GET_SINGLE(UIManager)->GetAction(actionStr);
		}
	};

	bindAction(eventInfo.ClickAction, pUI->OnClicked);
	bindAction(eventInfo.EnterAction, pUI->OnHoverEnter);
	bindAction(eventInfo.ExitAction, pUI->OnHoverExit);

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

void CLevelUIEditor::ApplyWorldSpaceRecursive(Engine::CUIObject* pUI, _bool bWorldSpace, _float scaleFactor)
{
	if (pUI == nullptr) return;

	// 1. 상태 변경
	pUI->SetWorldSpace(bWorldSpace);

	if (!bWorldSpace)
	{
		pUI->GetTransform().SetParentWorldMatrix(std::nullopt);

		pUI->CalcUICoord();
	}

	for (Engine::CHandle childHandle : pUI->GetChildren())
	{
		Engine::CUIObject* pChild = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(childHandle);
		ApplyWorldSpaceRecursive(pChild, bWorldSpace, scaleFactor);
	}
}


void CLevelUIEditor::StateView()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	
	if (!ImGui::CollapsingHeader("Global Properties", ImGuiTreeNodeFlags_DefaultOpen))
	return;
	
	// 부모 노드 이름 처리
	if (std::nullopt != Target_UI &&
		(nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI)))
	{
		Engine::CUIObject* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
		std::optional<CHandle> parentNode = selectUI->GetParent();
		if (parentNode != std::nullopt && 
			(nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*parentNode)))
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

		// SizeRatio
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("SizeRatio"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("##SizeRatio", &m_ScaleRatio, 0.001f, 0.2f, 2.f);
	
		// Alpha & Weight
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Alpha"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("##Alpha", &m_UIINFO.Alpha, 0.001f, 0.0f, 1.f);
	
		// weight
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Weight"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragInt("##Weight", &m_UIINFO.Weight, 1, 0, 100);

		// Rot
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Rot"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("##Rot", &m_UIINFO.Rot, 0.1f, -360, 360);
	
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
		static const char* UITypeNames[] = { "CONTAINER", "TEXUI", "FLIPBOOK", "TEXT", "BUTTON", "SPELLMETER", "HPBAR", "HPFILL",
			"LEFTHPFILL", "MINIMAP","SPELLBTN", "SHORTCUT_ICON", "DISOLVE", "GAMEOVERMASK", "VIDEOOBJ"};
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("UI Type"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(150);
		ImGui::Combo("##UIType", reinterpret_cast<int*>(&m_UIINFO.UIType), UITypeNames, IM_ARRAYSIZE(UITypeNames));
	
		static const char* EffectTypeNames[] = { "NONE", "HOVER", "CLICK" };
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Effect Type"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(150);
		ImGui::Combo("##EffectType", reinterpret_cast<int*>(&m_UIINFO.EffectType), EffectTypeNames, IM_ARRAYSIZE(EffectTypeNames));

		if (m_UIINFO.UIType == ETOUI(UI_TYPE::TEXT))
		{
			ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
			ImGui::Text("Text String"); ImGui::TableNextColumn();

			// 입력 칸이 셀 너비 전체를 차지하도록 설정
			ImGui::SetNextItemWidth(-FLT_MIN);

			ImGui::InputText("##TextData", m_cTextBuf, sizeof(m_cTextBuf));

			m_sText = m_cTextBuf;
			
			if (Target_UI != std::nullopt && 
				nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(*Target_UI))
			{
				CTextBox* pTextBox = E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(*Target_UI);
				pTextBox->SetwText(StringToWUTF8(m_sText));
			}
		}
	
		ImGui::EndTable();
	}

	//UpdateTargetState();
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
	_float localRot = selectInfo.LocalRot;
	_float scaleRatio = selectUI->GetScaleRatio();
	_float localScaleRatio = selectUI->GetLocalScaleRatio();
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

		// SizeRatio
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("SizeRatio"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("##SizeRatio", &scaleRatio, 0.001f, 0.2f, 2.f);

		// localSizeRatio
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("LocalScaleRatio"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("##LocalScaleRatio", &localScaleRatio, 0.001f, 0.2f, 2.f);

		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Rot"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("##Rot", &localRot, 0.1f, -360, 360);

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

		// weight
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("LocalRot"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(100); ImGui::DragFloat("##LocalRot", &m_UIINFO.LocalRot, 0.1f, -360, 360);

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
		static const char* UITypeNames[] = { "CONTAINER", "TEXUI", "FLIPBOOK", "TEXT", "BUTTON", "SPELLMETER", "HPBAR"
			, "HPFILL", "LEFTHPFILL", "MINIMAP", "SPELLBTN", "SHORTCUT_ICON", "DISOLVE", "GAMEOVERMASK", "VIDEOOBJ" };
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("UI Type"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(150);
		ImGui::Combo("##LUIType", reinterpret_cast<int*>(&m_UIINFO.UIType), UITypeNames, IM_ARRAYSIZE(UITypeNames));

		static const char* EffectTypeNames[] = { "NONE", "HOVER", "CLICK" };
		ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
		ImGui::Text("Effect Type"); ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(150);
		ImGui::Combo("##LEffectType", reinterpret_cast<int*>(&m_UIINFO.EffectType), EffectTypeNames, IM_ARRAYSIZE(EffectTypeNames));

		if (m_UIINFO.UIType == ETOUI(UI_TYPE::TEXT))
		{
			ImGui::TableNextRow(); ImGui::TableNextColumn(); ImGui::AlignTextToFramePadding();
			ImGui::Text("Text String"); ImGui::TableNextColumn();

			// 입력 칸이 셀 너비 전체를 차지하도록 설정
			ImGui::SetNextItemWidth(-FLT_MIN);

			ImGui::InputText("##TextData", m_cTextBuf, sizeof(m_cTextBuf));

			m_sText = m_cTextBuf;

			if (Target_UI != std::nullopt &&
				nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(*Target_UI))
			{
				CTextBox* pTextBox = E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(*Target_UI);
				pTextBox->SetwText(StringToWUTF8(m_sText));
			}
		}

		ImGui::EndTable();
	}

	// Update Target Properties
	selectInfo.LocalX = localX;
	selectInfo.LocalY = localY;
	selectInfo.WidthRatioX = widthX;
	selectInfo.WidthRatioY = widthY;
	selectInfo.AlphaRatio = alphaRatio;
	selectInfo.WeightOffset = weightOffset;
	selectUI->SetScaleRatio(scaleRatio);
	m_ScaleRatio = scaleRatio;
	selectInfo.LocalRot = localRot;
	m_LocalScaleRatio = localScaleRatio;
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
				printf("로드 대상 파일: %s\n", file.fullPath.c_str());		  			}
		}
	}

	ImGui::EndChild();
	ImGui::End();
}

void CLevelUIEditor::UpdateTargetState()
{
	if (std::nullopt != Target_UI && 
		nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI))
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
		selectInfo.Rot = m_UIINFO.Rot;
		selectUI->SetScaleRatio(m_ScaleRatio);
		selectUI->SetLocalScaleRatio(m_LocalScaleRatio);

		if (ETOUI(UI_TYPE::FLIPBOOK) == *selectUI->GetUIType())
		{
			FLIP_INFO& flipInfo = static_cast<CFlipbookUI*>(selectUI)->GetFlipInfo();

			m_FLIPINFO.cellsize = flipInfo.cellsize;
			m_FLIPINFO.Duration = flipInfo.Duration;
			m_FLIPINFO.TotalFrame = flipInfo.TotalFrame;
			m_FLIPINFO.Padding = flipInfo.Padding;
		}
		selectUI->CalcUICoord();

		if (!selectUI->GetWorldSpace())
		{
			selectUI->CalcUICoord();
		}
		else
		{
			selectUI->GetTransform().SetScale(E::_float3{ selectInfo.SizeX * 0.01f, selectInfo.SizeY * 0.01f, 1.f });
		}
	}
}

void CLevelUIEditor::DrawHierarchyNode(CHandle uiHandle)
{
	Engine::CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(uiHandle);
	if (pUI == nullptr) return;

	// 1. 트리 노드 스타일 플래그 설정
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
		| ImGuiTreeNodeFlags_OpenOnDoubleClick
		| ImGuiTreeNodeFlags_SpanAvailWidth;

	// 현재 선택된 UI라면 파란색으로 하이라이트
	if (Target_UI.has_value() && Target_UI.value() == uiHandle)
		flags |= ImGuiTreeNodeFlags_Selected;

	// 자식이 없으면 잎(Leaf) 노드로 설정해서 열기 화살표를 숨김
	const auto& children = pUI->GetChildren();
	if (children.empty())
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

	// 2. 트리 노드 그리기 (객체 포인터를 ID로 사용해서 이름이 겹쳐도 버그 안 나게 함)
	bool bIsOpen = ImGui::TreeNodeEx((void*)pUI, flags, "%s", pUI->GetName());

	// 3. 노드를 클릭했을 때 Target_UI 변경 
	// (IsItemToggledOpen을 체크해서 화살표를 눌러서 트리를 열 때는 선택되지 않게 방지)
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		ResetProperty(uiHandle);
	}

	// 4. 자식이 있고, 트리가 열려있다면 재귀 호출로 자식들을 쭉 그림
	if (bIsOpen && !children.empty())
	{
		for (CHandle childHandle : children)
		{
			DrawHierarchyNode(childHandle);
		}
		ImGui::TreePop(); // 자식이 있는 노드가 열려있을 때만 Pop 해줌
	}
}

void CLevelUIEditor::ResetProperty(std::optional<Engine::CHandle> newTargetHandle)
{
	Target_UI = newTargetHandle;

	if (Target_UI == std::nullopt)
	{
		m_UIINFO = UI_INFO{};		
		m_FLIPINFO = FLIP_INFO{};
		return;
	}

	Engine::CUIObject* pTargetUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*Target_UI);
	if (pTargetUI == nullptr)
		return;

	m_UIINFO = pTargetUI->GetUIInfo();
	strcpy_s(m_cName, sizeof(m_cName), m_UIINFO.Name.c_str());

	m_ScaleRatio = pTargetUI->GetScaleRatio();
	m_LocalScaleRatio = pTargetUI->GetLocalScaleRatio();

	if (*pTargetUI->GetUIType() == ETOUI(UI_TYPE::FLIPBOOK))
	{
		CFlipbookUI* pFlipbook = static_cast<CFlipbookUI*>(pTargetUI);
		m_FLIPINFO = pFlipbook->GetFlipInfo();
	}
	else if (*pTargetUI->GetUIType() == ETOUI(UI_TYPE::TEXUI))
	{
	}
	else if (*pTargetUI->GetUIType() == ETOUI(UI_TYPE::TEXT))
	{
		CTextBox* textBox = static_cast<CTextBox*>(pTargetUI);
		m_sText = WStringToUTF8(textBox->GetwText());
		strcpy_s(m_cTextBuf, sizeof(m_cTextBuf), m_sText.c_str());
	}

	m_WorldPos = pTargetUI->GetTransform().GetPosition();
	m_IsWorldSpace = pTargetUI->GetWorldSpace();
}

void CLevelUIEditor::ClearUI()
{
	std::vector<CHandle> uiHandles = GET_SINGLE(UIManager)->GetRootUIHandles();
	for (auto handle : uiHandles)
	{
		GET_SINGLE(UIManager)->DeleteUIRecursive(handle);
	}
}

Engine::UPtr<CLevelUIEditor> CLevelUIEditor::Create()
{
	auto pInstance = Engine::UPtr<CLevelUIEditor>(new CLevelUIEditor{});
	pInstance->SetDeferredInitialization();
	return pInstance;
}

void CLevelUIEditor::Free()
{
	CLevel::Free();
}
