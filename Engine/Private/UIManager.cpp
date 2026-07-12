#include "pch.h"
#include "UIManager.h"
#include "GameInstance.h"
#include "UIObject.h"
#include "FlipbookUI.h"

CUIManager::CUIManager()
{
}

CUIManager::~CUIManager()
{
}

void CUIManager::UpdateGUI()
{
	ImGui::Begin("CUIManager");



	ImGui::End();

}

void CUIManager::Load()
{
}

void CUIManager::Find_UiDesc()
{
}

void CUIManager::LoadPrefab(char path[256])
{
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

E::CUIObject* CUIManager::LoadUIRecursive(const nlohmann::ordered_json& obj, CUIObject* parent)
{
	int uiType = obj["UiType"];

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
			flipInfo.cellsize = obj["CellSize"];
			flipInfo.TotalFrame = obj["TotalFrame"];
			flipInfo.Padding = obj["Padding"];
			flipInfo.Duration = obj["Duration"];
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

UPtr<CUIManager> CUIManager::Create()
{
	return UPtr<CUIManager>(new CUIManager{});
}
