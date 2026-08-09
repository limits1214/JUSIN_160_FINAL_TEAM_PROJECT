#include "pch.h"
#include "BTDecorator.h"

CBTDecorator::CBTDecorator()
{
}

CBTDecorator::CBTDecorator(const CBTDecorator& Prototype) : CBTRoot(Prototype)
{
}

CBTDecorator ::~CBTDecorator()
{
}


HRESULT CBTDecorator::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	
	return S_OK;
}

HRESULT CBTDecorator::Initalize(void* pArg)
{
    if (FAILED(__super::Initalize(pArg)))
        return E_FAIL;

	m_GuiNode.vColor = { 1.0f,1.f, 1.f,1 };
    return S_OK;
}

HRESULT CBTDecorator::Save_SubTree(const _string& SavePath)
{
	if (nullptr == m_pDecorator)
	{
		MSG_BOX("Nob");
		return E_FAIL;
	}

	nlohmann::json j;
	j = Save_Node();

	std::ofstream path(SavePath);
	path << j.dump(4);
	path.close();

	return S_OK;
	
}

EVALUATE CBTDecorator::Evaluate(_float fTimeDelta)
{
	if (m_pDecorator != nullptr)
		return m_eDebug = m_pDecorator->Execute(fTimeDelta);

    return 	m_eDebug = EVALUATE::FAILED;
}
void CBTDecorator::Abort()
{
	if (m_pDecorator)
		m_pDecorator->AbortExecute();
}
void CBTDecorator::ResetDebug()
{
	m_eDebug = EVALUATE::END;
	if (m_pDecorator != nullptr)
		m_pDecorator->ResetDebug();
}


void CBTDecorator::OnEnter()
{

}
void CBTDecorator::OnExit(EVALUATE eResult)
{
	if (m_pDecorator)
		m_pDecorator->AbortExecute();
}
nlohmann::json CBTDecorator::Save_Node()
{
	const _string Name = "Child";
	nlohmann::json j;
	j = __super::Save_Node();

	if (m_GuiLink.SlotEnd[0].iDestNode != -1)
	{
		JsonSaveLoadManager::SaveJsonTypeString(j, "LinkEndSlotName", m_GuiLink.SlotEnd[0].DestName);
		SaveJsonValue(j, "LinkEndSlotID", m_GuiLink.SlotEnd[0].iDestNode);
		SaveJsonEnum(j, "LinkEndSlotEnum", m_GuiLink.SlotEnd[0].eType);
	}	
	if(m_pDecorator != nullptr)
		j[Name] = m_pDecorator->Save_Node();
	
	return j;
}

HRESULT CBTDecorator::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	m_GuiLink.SlotEnd.resize(1);

	JsonSaveLoadManager::LoadJsonTypeString(j, "LinkEndSlotName", m_GuiLink.SlotEnd[0].DestName);
	LoadJsonValue(j, "LinkEndSlotID", m_GuiLink.SlotEnd[0].iDestNode);
	LoadJsonEnum(j, "LinkEndSlotEnum", m_GuiLink.SlotEnd[0].eType);
	
	if (j.contains("Child"))
	{
		_string MasterName{};
		NODEGROUP eGroup{};
		if (JsonSaveLoadManager::LoadJsonTypeString(j["Child"], "MasterName", MasterName))
		{
			if (LoadJsonEnum(j["Child"], "Group", eGroup))
			{
				auto pSrc = engine_uptr_cast<CBTRoot>(CGameInstance::Get().ClonePrototype(eGroup, MasterName, nullptr));
				pSrc->Load_json(j["Child"]);
				m_pDecorator = std::move(pSrc);
			}

		}
	}
	

    return S_OK;
}

