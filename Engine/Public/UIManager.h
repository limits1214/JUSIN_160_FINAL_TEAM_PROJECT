#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CUIObject;

class CUIManager final : public CEngineBase
{
private:
	CUIManager();
	~CUIManager();

public:
	void UpdateGUI();

	void Load();

	void Find_UiDesc();

	void LoadPrefab(char path[256]);
	E::CUIObject* LoadUIRecursive(const nlohmann::ordered_json& obj, CUIObject* parent);

private:
	std::vector<UI_DESC> Ui_Desces{};
public:
	static UPtr<CUIManager> Create();
};

NS_END
