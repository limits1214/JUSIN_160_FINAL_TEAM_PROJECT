#include "pch.h"
#include "BTSubTreeNode.h"
#include "BTComposite.h"
#include "BTDecorator.h"
#include "ComBeHavior.h"
CBTSubTreeNode::CBTSubTreeNode()
{
}

CBTSubTreeNode::CBTSubTreeNode(const CBTSubTreeNode& pPrototype) : CBTActionNode(pPrototype)
{
	m_strResMajor = pPrototype.m_strResMajor;
	m_strResMinor = pPrototype.m_strResMinor;
	m_pSubTreeRoot = nullptr;
}


CBTSubTreeNode::~CBTSubTreeNode()
{
}


HRESULT CBTSubTreeNode::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTSubTree";
	return S_OK;
}

HRESULT CBTSubTreeNode::Initalize(void* pArg)
{
	auto pDesc = static_cast<ACTION_NODE_DESC*>(pArg);

	if (FAILED(__super::Initalize(pArg)))
		return E_FAIL;

	return S_OK;
}

void CBTSubTreeNode::Abort()
{
	if (nullptr == m_pSubTreeRoot)
		return;

	m_pSubTreeRoot->AbortExecute();
}

void CBTSubTreeNode::Update_Gui()
{
	if(nullptr != m_pSubTreeRoot)
		m_pSubTreeRoot->Get_GuiNodeLink().ParentNode.Reset();

	if (ImGui::Button("Show SubTree"))
		m_bIsSubTree = !m_bIsSubTree;
	if (ImGui::TreeNode("SubTree Select"))
	{
		if (ImGui::Button("Open Major"))
			m_bMajorPop = !m_bMajorPop;

		if (ImGui::Button("Open Minor"))
			m_bMinorPop = !m_bMinorPop;

		if (m_bMajorPop)
			InputPopUp("BeHavior MajorName", m_strResMajor, m_bMajorPop);
		if (m_bMinorPop)
			InputPopUp("BeHavior MinorName", m_strResMinor, m_bMinorPop);

		ImGui::Text("BeHavior MajorName : %s", m_strResMajor.c_str());
		ImGui::Text("BeHavior MinorName : %s", m_strResMinor.c_str());

		if(ImGui::Button("Create SubTree"))
			if (!m_strResMajor.empty() && !m_strResMinor.empty())
				CreateSubTree();
		ImGui::TreePop();
	}
}

void		CBTSubTreeNode::OnEnter()
{

}
void		CBTSubTreeNode::OnExit(EVALUATE eResult)
{

}

void CBTSubTreeNode::CreateSubTree()
{
	//일단 서브트리 리소스 불러오고
	
	auto pRes = CGameInstance::Get().GetResourceFirst<CResJson>(m_strResMajor, m_strResMinor);
	if (nullptr == pRes) return ;

	if (nullptr == pRes)
	{
		MSG_BOX("Load Failed Json To SubTree");
		return;
	}
	_string Test = pRes->GetPath();
	m_pSubTreeRoot.reset();

	nlohmann::json j = pRes->Get_Json();
	
	NODEGROUP eGroup{};
	_string MasterName{};
	LoadJsonEnum(j, "Group", eGroup);
	JsonSaveLoadManager::LoadJsonTypeString(j, "MasterName", MasterName);

	auto pSrc = engine_uptr_cast<CBTRoot>(CGameInstance::Get().ClonePrototype(eGroup, MasterName));
	if (nullptr == pSrc)
		return;
	pSrc->Load_json(pRes->Get_Json());

	m_pSubTreeRoot = std::move(pSrc);
	if (nullptr == m_pSubTreeRoot)
		return;
	auto pBT = Get_ComBT();
	if (nullptr == pBT)
		return;

	ConnectNode(m_pSubTreeRoot.get(), -1,pBT);

	m_pSubTreeRoot->Get_GuiNodeLink().ParentNode.Reset();
}

EVALUATE CBTSubTreeNode::Evaluate(_float fTimeDelta)
{
	if (nullptr == m_pSubTreeRoot)
		return m_eDebug = EVALUATE::FAILED;

	return m_pSubTreeRoot->Execute(fTimeDelta);
}

nlohmann::json CBTSubTreeNode::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	JsonSaveLoadManager::SaveJsonTypeString(j, "SubTreeResMajor", m_strResMajor);
	JsonSaveLoadManager::SaveJsonTypeString(j, "SubTreeResMinor", m_strResMinor);
	return j;
}

HRESULT CBTSubTreeNode::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);

	_bool bMajor = JsonSaveLoadManager::LoadJsonTypeString(j, "SubTreeResMajor", m_strResMajor);
	_bool bMinor = JsonSaveLoadManager::LoadJsonTypeString(j, "SubTreeResMinor", m_strResMinor);


	if (bMajor && bMinor)
	{
		m_pSubTreeRoot.reset();

		auto pRes = CGameInstance::Get().GetResourceFirst<CResJson>(m_strResMajor, m_strResMinor);
		if (nullptr == pRes)
		{
			MSG_BOX("Load Failed Json To SubTree");
			return E_FAIL;
		}
		NODEGROUP eGroup{};
		_string MasterName{};
		nlohmann::json jSub = pRes->Get_Json();

		LoadJsonEnum(jSub, "Group", eGroup);
		JsonSaveLoadManager::LoadJsonTypeString(jSub, "MasterName", MasterName);
		auto pSrc = engine_uptr_cast<CBTRoot>(CGameInstance::Get().ClonePrototype(eGroup, MasterName));
		if (nullptr == pSrc)
			return E_FAIL;
		pSrc->Load_json(jSub);

		m_pSubTreeRoot = std::move(pSrc);
		if (nullptr == m_pSubTreeRoot)
			return E_FAIL;
	}
		
	return S_OK;
}
void CBTSubTreeNode::ResetDebug()
{
	__super::ResetDebug();

	if (nullptr != m_pSubTreeRoot)
		m_pSubTreeRoot->ResetDebug();
}
CBTRoot* CBTSubTreeNode::Get_SubTreeNode()
{
	if (nullptr == m_pSubTreeRoot)
		return nullptr;

	return m_pSubTreeRoot.get();
}

void CBTSubTreeNode::ConnectNode(CBTRoot* pRoot,  int32_t iPreIndex , class CComBeHavior* pBehavior, CBTRoot* pParent)
{

	if (nullptr == pRoot)
		return;
	
	pRoot->Get_GuiNodeInfo().iID = pBehavior->Get_NodeID()++;
	pRoot->Set_OwnerName(m_OwnerName);
	pRoot->Set_Handle(m_Handle);
	pBehavior->RegistNode(pRoot->Get_GuiNodeInfo().iID, pRoot);

	BEHAVIOR eType = pRoot->Get_GuiNodeInfo().eMyType;

	if (pParent != nullptr)
	{
		pParent->Get_GuiNodeLink().SlotEnd[iPreIndex] = pRoot->Get_GuiNodeInfo().Get_DestInfo();
	}
	if (eType == BEHAVIOR::SELECTOR || eType == BEHAVIOR::SECQUNCE || eType == BEHAVIOR::RAND_SELECTOR)
	{
		auto pSrc = static_cast<CBTComposite*>(pRoot)->Get_Nodes();
		if (nullptr == pSrc)
			return;

		for (auto& pNode : *pSrc)
		{
			if (nullptr != pNode)
			{
				pNode->Get_GuiNodeLink().ParentNode = pRoot->Get_GuiNodeInfo().Get_DestInfo();
				ConnectNode(pNode.get(), pNode->Get_GuiNodeLink().iStartIdx, pBehavior, pRoot);
			}
				
		}
	}
	else if (eType == BEHAVIOR::DECORATOR)
	{
		auto pSrc = static_cast<CBTDecorator*>(pRoot);
		
		auto& pChild = pSrc->Get_Child();
		if (nullptr == pChild)
			return;

		pChild->Get_GuiNodeLink().ParentNode = pRoot->Get_GuiNodeInfo().Get_DestInfo();
		pRoot->Get_GuiNodeLink().SlotEnd.resize(1);
	
		ConnectNode(pChild.get(), iPreIndex, pBehavior, pRoot);
	}
}

void CBTSubTreeNode::InputPopUp(const _string& strPopupName,  _string& strTagName, _bool& bPopUp)
{
	ImGui::OpenPopup(strPopupName.c_str());

	_char NameBUffer[64]{};
	if (ImGui::BeginPopup(strPopupName.c_str(), ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("FileName");
		if (ImGui::InputText("##FileName", &NameBUffer[0], IM_ARRAYSIZE(NameBUffer))) //이름 입력
		{
			strTagName = NameBUffer;

		}


		if (ImGui::Button("Ok"))
		{
			bPopUp = false;
			if (strTagName.empty())
			{

				ImGui::CloseCurrentPopup();
				MSG_BOX("NoName");
			}
						
		} ImGui::SameLine(100.f);
		if (ImGui::Button("Cancle"))
		{
			bPopUp = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}
UPtr<CBTSubTreeNode> CBTSubTreeNode::Create(void* pArg)
{
	auto pInstance = ToUPtr(new CBTSubTreeNode());
	if (FAILED(pInstance->InitializePrototype(pArg)))
	{
		MSG_BOX("Failed to Created : CBTSubTreeNode");
		return nullptr;
	}
	return pInstance;
}
UPtr<CPrototype> CBTSubTreeNode::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTSubTreeNode{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTSubTreeNode");
		return nullptr;
	}

	return pInstance;
}

