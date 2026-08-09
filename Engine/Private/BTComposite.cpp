#include "pch.h"
#include "BTComposite.h"
#include "BTSelector.h"
#include "BTSecqunce.h"
#include "BTActionNode.h"
#include "BTDecorator.h"
CBTComposite::CBTComposite()
{

}
CBTComposite::CBTComposite(const CBTComposite& rhs) : CBTRoot(rhs), m_Actions{  }
{
}
CBTComposite::~CBTComposite()
{

}

HRESULT CBTComposite::Save_SubTree(const _string& SavePath)
{
	if (m_Actions.empty() || m_Actions.front() == nullptr)
	{
		MSG_BOX("Nob Front null");
		return E_FAIL;
	}

	nlohmann::json j;
	j = Save_Node();

	std::ofstream path(SavePath);
	path << j.dump(4);
	path.close();

	return S_OK;
}

void CBTComposite::Abort()
{
}

void CBTComposite::Tick(_float fTimeDelta)
{
	for (auto& iter : m_Actions)
	{
		if(iter != nullptr)
		iter->ResetDebug();
	}
	for (auto& iter : m_Actions)
	{
		if(iter != nullptr)
		iter->Execute(fTimeDelta);
	}
		
}

void CBTComposite::ResetDebug()
{
	m_eDebug = EVALUATE::END;
	for (auto& iter : m_Actions)
	{
		if(nullptr != iter)
			iter->ResetDebug();
	}
		
}

HRESULT CBTComposite::Add_Node(uint32_t iIndex, UPtr<CBTRoot> pNode)
{
	if (iIndex >= m_Actions.size())
		return E_FAIL;

	if (m_Actions[iIndex] != nullptr)
		return E_FAIL;
	
	m_Actions[iIndex] = std::move(pNode);
	return S_OK;
}

nlohmann::json CBTComposite::Save_Node()
{
	std::vector<UPtr<CBTRoot>> compactActions;
	std::vector<DEST_NODE> compactSlots;

	compactActions.reserve(m_Actions.size());
	compactSlots.reserve(m_Actions.size());

	for (auto& pAction : m_Actions)
	{
		if (pAction == nullptr)
			continue;

		const size_t iNewIndex = compactActions.size();

		pAction->Get_GuiNodeLink().ParentNode = m_GuiNode.Get_DestInfo();
		pAction->Get_GuiNodeLink().iStartIdx = static_cast<int32_t>(iNewIndex);

		compactSlots.push_back(pAction->Get_GuiNodeInfo().Get_DestInfo());
		compactActions.push_back(std::move(pAction));
	}

	m_Actions = std::move(compactActions);
	m_GuiLink.SlotEnd = std::move(compactSlots);

	nlohmann::json j = __super::Save_Node();

	for (size_t i = 0; i < m_GuiLink.SlotEnd.size(); ++i)
	{
		const _string destSlotName = "LinkEndSlotName" + std::to_string(i);
		const _string destID = "LinkEndSlotID" + std::to_string(i);
		const _string slotType = "LinkEndSlotType" + std::to_string(i);

		JsonSaveLoadManager::SaveJsonTypeString(
			j, destSlotName, m_GuiLink.SlotEnd[i].DestName);
		SaveJsonValue(j, destID, m_GuiLink.SlotEnd[i].iDestNode);
		SaveJsonEnum(j, slotType, m_GuiLink.SlotEnd[i].eType);
	}

	j["GuiLink_SlotSize"] = m_GuiLink.SlotEnd.size();
	j["Child"] = nlohmann::json::array();

	for (auto& pAction : m_Actions)
		j["Child"].push_back(pAction->Save_Node());

	j["Size"] = m_Actions.size();
	return j;
}
HRESULT		CBTComposite::Load_json(const nlohmann::json& j)
{
	if (FAILED(__super::Load_json(j)))
		return E_FAIL;

	const size_t iArraySize = j["Size"];

	m_Actions.clear();
	m_GuiLink.SlotEnd.clear();
	m_Actions.resize(iArraySize);
	m_GuiLink.SlotEnd.resize(iArraySize);

	for (size_t i = 0; i < iArraySize; ++i)
	{
		_string masterName{};
		NODEGROUP eGroup{};

		if (!JsonSaveLoadManager::LoadJsonTypeString(
				j["Child"][i], "MasterName", masterName))
			continue;

		if (!LoadJsonEnum(j["Child"][i], "Group", eGroup))
			continue;

		auto pChild = engine_uptr_cast<CBTRoot>(
			CGameInstance::Get().ClonePrototype(eGroup, masterName, nullptr));

		if (pChild == nullptr)
			continue;

		if (FAILED(pChild->Load_json(j["Child"][i])))
			continue;

		pChild->Get_GuiNodeLink().ParentNode = m_GuiNode.Get_DestInfo();
		pChild->Get_GuiNodeLink().iStartIdx = static_cast<int32_t>(i);
		m_GuiLink.SlotEnd[i] = pChild->Get_GuiNodeInfo().Get_DestInfo();
		m_Actions[i] = std::move(pChild);
	}
	return S_OK;
}
HRESULT CBTComposite::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	m_Actions.resize(m_GuiLink.SlotEnd.size());
	return S_OK;
}
UPtr<CBTComposite> CBTComposite::Create(void* pArg)
{
	auto pInstance = ToUPtr(new CBTComposite());
	if (FAILED(pInstance->InitializePrototype(pArg)))
	{
		MSG_BOX("Failed to Created : CBTComposite");
		return nullptr;
	}
	return pInstance;
}


E::UPtr<E::CPrototype> CBTComposite::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTComposite{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTComposite");
		return nullptr;
	}

	return pInstance;
}
