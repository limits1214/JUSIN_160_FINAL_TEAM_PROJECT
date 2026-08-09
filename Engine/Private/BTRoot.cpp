#include "pch.h"
#include "BTRoot.h"

CBTRoot::CBTRoot()
{
}

CBTRoot::CBTRoot(const CBTRoot& rhs) : CPrototype(rhs)
{
	m_eGroup = rhs.m_eGroup;
	m_MasterName = rhs.m_MasterName;
	m_bEntered = false;
}

CBTRoot::~CBTRoot()
{
}

HRESULT CBTRoot::InitializePrototype(void* pArg)
{
	
	return S_OK;
}

HRESULT CBTRoot::Initalize(void* pArg)
{
	auto pDesc = static_cast<BTROOT_DESC*>(pArg);
	if (nullptr != pDesc)
	{
		m_Handle = pDesc->Handle;
		m_GuiNode = std::move(pDesc->m_GuiNode);
		m_GuiLink = std::move(pDesc->m_GuiLink);
	}
	return S_OK;
}

HRESULT CBTRoot::Save_SubTree(const _string& SavePath)
{
	return S_OK;
}


nlohmann::json CBTRoot::Save_Node()
{
	nlohmann::json j;

	SaveJsonValue(j, "ID", m_GuiNode.iID);
	SaveJsonValue(j, "fValue", m_GuiNode.fValue);
	SaveJsonValue(j, "Abort", m_GuiNode.bAbort);

	SaveJsonEnum(j, "Group", m_eGroup);
	SaveJsonEnum(j, "GuiNode_BeHaviorType", m_GuiNode.eMyType);
	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "GuiNode_Pos", m_GuiNode.vPos);
	JsonSaveLoadManager::SaveJsonTypeFloat2(j, "GuiNode_Size", m_GuiNode.vSize);
	JsonSaveLoadManager::SaveJsonTypeFloat4(j, "GuiNode_Color", m_GuiNode.vColor);
	JsonSaveLoadManager::SaveJsonTypeString(j, "GuiNode_Name", m_GuiNode.Name);

	SaveJsonEnum(j, "GuiLink_ParentNodeEnum", m_GuiLink.ParentNode.eType);
	SaveJsonValue(j, "GuiLink_StartIndex", m_GuiLink.iStartIdx);
	SaveJsonValue(j, "GuiLink_StartParentNode", m_GuiLink.ParentNode);
	JsonSaveLoadManager::SaveJsonTypeString(j, "MasterName", m_MasterName);
	return j;
}
HRESULT				CBTRoot::Load_json(const nlohmann::json& j)
{
	LoadJsonValue(j, "ID", m_GuiNode.iID);
	LoadJsonValue(j, "fValue", m_GuiNode.fValue);
	LoadJsonValue(j, "Abort", m_GuiNode.bAbort);
	LoadJsonEnum(j, "Group", m_eGroup);
	LoadJsonEnum(j, "GuiNode_BeHaviorType", m_GuiNode.eMyType);
	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "GuiNode_Pos", m_GuiNode.vPos);
	JsonSaveLoadManager::LoadJsonTypeFloat2(j, "GuiNode_Size", m_GuiNode.vSize);
	JsonSaveLoadManager::LoadJsonTypeFloat4(j, "GuiNode_Color", m_GuiNode.vColor);
	JsonSaveLoadManager::LoadJsonTypeString(j, "GuiNode_Name", m_GuiNode.Name);
	
	if (m_GuiNode.Name == "Selector Root")
		int32_t i = 0;
	LoadJsonEnum(j, "GuiLink_ParentNodeEnum", m_GuiLink.ParentNode.eType);
	LoadJsonValue(j, "GuiLink_StartIndex", m_GuiLink.iStartIdx);
	LoadJsonValue(j, "GuiLink_StartParentNode", m_GuiLink.ParentNode);
	JsonSaveLoadManager::LoadJsonTypeString(j, "MasterName", m_MasterName);
	return S_OK;
}
void CBTRoot::Set_Flag(uint32_t iFlag, FLAGTYPE eType)
{
	if (auto iter = Get_ComBT())
	{
		iter->Set_Flag(iFlag, eType);
	}
}
void CBTRoot::AbortExecute()
{
	 // 기존 각 노드의 Abort 로직 유지
	if (m_bEntered)
	{
		Abort();
		OnExit(EVALUATE::FAILED);
		m_bEntered = false;
	}
}
EVALUATE CBTRoot::Execute(_float fTimeDelta)
{
	if (!m_bEntered)
	{
		OnEnter();
		m_bEntered = true;
	}
	const EVALUATE eResult = Evaluate(fTimeDelta);

	m_eDebug = eResult;

	if (eResult != EVALUATE::RUN)
	{
		OnExit(eResult);
		m_bEntered = false;
	}

	return eResult;
}
CComBeHavior* CBTRoot::Get_ComBT()
{
	if (auto pObj = CGameInstance::Get().GetGameObjectByHandle(m_Handle))
	{
		if (auto pComBt = pObj->GetComponent<CComBeHavior>(m_OwnerName))
		{
			return pComBt;
		}
	}
	return nullptr;

}
_bool CBTRoot::Check_Flag(uint32_t iFlag)
{
	if (auto pObj = CGameInstance::Get().GetGameObjectByHandle(m_Handle))
	{
		if (auto pComBt = pObj->GetComponent<CComBeHavior>(m_OwnerName))
		{
			return pComBt->Check_Flag(iFlag);
		}
	}
	return false;
}

uint32_t CBTRoot::Get_Flag()
{
	if (auto pObj = CGameInstance::Get().GetGameObjectByHandle(m_Handle))
	{
		if (auto pComBt = pObj->GetComponent<CComBeHavior>(m_OwnerName))
		{
			return pComBt->Get_Flag();
		}
	}
	return 0;
}
