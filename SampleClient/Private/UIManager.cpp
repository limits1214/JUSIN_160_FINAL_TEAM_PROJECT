#include "pch.h"
#include "GameInstance.h"
#include "UIManager.h"
#include "TextureUI.h"
#include "EffectUI.h"
#include "TextBox.h"
#include "Button.h"
#include <fstream>

NS_USING(Client)

UIManager::~UIManager()
{

}

void UIManager::Update()
{
}

void UIManager::InitializeActions()
{
	// 크기를 1.1배로 키우는 애니메이션 액션
	m_EventMap["ScaleUp"] = [](CUIObject* pCaller)
	{
		if (pCaller == nullptr) return;

		// 1. 타겟 UI의 트윈 컴포넌트를 가져옵니다. 
		// (pCaller에 GetTweenCom() 같은 Getter가 있다고 가정)
		auto pTween = pCaller->GetTweenCom();
		if (pTween != nullptr)
		{
			float startScale = pCaller->GetScaleRatio();

			// 2. 콜백 내부 람다에서 pCaller를 캡처([pCaller])하여 사용합니다.
			pTween->PlayTween(startScale, 1.1f, 0.1f,
				[pCaller](float currentValue) {
					pCaller->SetScaleRatio(currentValue);
					pCaller->CalcUICoord();
				});
		}
	};

	// 알파값을 서서히 올리는 애니메이션 액션
	m_EventMap["FadeIn"] = [](CUIObject* pCaller)
	{
		if (pCaller == nullptr) return;

		auto pTween = pCaller->GetTweenCom();
		if (pTween != nullptr)
		{
			float startAlpha = pCaller->GetAlphaRatio();

			pTween->PlayTween(startAlpha, 1.0f, 0.3f,
				[pCaller](float currentValue) {
					pCaller->SetAlphaRatio(currentValue);
				});
		}
	};
}

std::function<void(CUIObject* pCaller)> UIManager::GetAction(const std::string& actionName)
{
	auto iter = m_EventMap.find(actionName);
	if (iter != m_EventMap.end())
		return iter->second;

	return nullptr;
}

std::optional<CHandle> UIManager::LoadPrefab(std::string name, std::string g_BasePath)
{
	char path[256] = "";
	strcpy_s(path, sizeof(path), g_BasePath.c_str());
	strcat_s(path, sizeof(path), name.c_str());
	strcat_s(path, sizeof(path), ".json");

	std::ifstream file(path);

	if (!file.is_open())
	{
		MSG_BOX("파일 열기 실패");
		return std::nullopt;
	}

	nlohmann::ordered_json root;
	file >> root;
	file.close();

	for (const auto& obj : root["UI"])
	{
		LoadUIRecursive(obj, nullptr);
	}

	return m_rootHandle;
}

E::CUIObject* UIManager::LoadUIRecursive(const nlohmann::ordered_json& obj, E::CUIObject* parent)
{
	int uiType = obj["UiType"];

	E::CUIObject* pUI = nullptr;

	E::CUIObject::UIOBJECT_DESC Desc{};
	std::optional<CHandle> uiHandle = std::nullopt;

	Desc.sObjectTag = obj["Name"];

	int EffectType = obj["UI_EFFECT_TYPE"];

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

		if (EffectType == ETOUI(UI_EFFECT_TYPE::HOVER))
		{
			pUI->SetActive(false);
			if (parent &&
				*parent->GetUIType() == ETOUI(UI_TYPE::BUTTON))
			{
				static_cast<CButton*>(parent)->SetEffectHovered(*uiHandle);
			}
		}
		else if (EffectType == ETOUI(UI_EFFECT_TYPE::CLICK))
		{
			if (parent &&
				*parent->GetUIType() == ETOUI(UI_TYPE::BUTTON))
			{
				static_cast<CButton*>(parent)->SetEffectClicked(*uiHandle);
			}
		}
		break;
	case ETOUI(UI_TYPE::TEXT):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_TextBox", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(*uiHandle);
		{
			TEXT_INFO& textInfo = static_cast<CTextBox*>(pUI)->GetTextInfo();
			//textInfo.Text = obj["Text"];
		}
		break;
	case ETOUI(UI_TYPE::BUTTON):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_UIEDITOR", "Prototype_GameObject_Button", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CButton>(*uiHandle);
		break;
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
		m_rootHandle = uiHandle;

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

void UIManager::DeleteUIRecursive(std::optional<CHandle> targetHandle)
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

	targetUI->SetPendingDestroyCascade();

	return;
}
