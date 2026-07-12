#pragma once
#include "Client_Defines.h"
#include "UIObject.h"

NS_BEGIN(Client)

class UIManager
{
	DECLARE_SINGLE(UIManager)

	~UIManager();

	void	Update();

public:
	void InitializeActions();
	std::function<void(CUIObject* pCaller)> GetAction(const std::string& actionName);

private:
	std::map<std::string, std::function<void(class CUIObject*)>> m_EventMap;

public:
	std::optional<CHandle> LoadPrefab(std::string name, std::string g_BasePath = "./Resources/SampleClient/UIData/Prefabs/");
	E::CUIObject* LoadUIRecursive(const nlohmann::ordered_json& obj, E::CUIObject* parent);
	void DeleteUIRecursive(std::optional<CHandle> targetHandle);

private:
	std::string g_BasePath = "./Resources/SampleClient/UIData/Prefabs/";
	std::optional<CHandle> m_rootHandle = std::nullopt;
};

NS_END
