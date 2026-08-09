#include "pch.h"
#include "ComBeHavior.h"
#include "BTDecorator.h"
#include "BTComposite.h"
#include "BTBlackboard.h"
#include "BTSubTreeNode.h"
CComBeHavior::CComBeHavior()
{
}

CComBeHavior::CComBeHavior(const CComBeHavior& Prototype) : CComponent(Prototype)
{
}

CComBeHavior::~CComBeHavior()
{
}

HRESULT CComBeHavior::InitializePrototype(void* pArg)
{
    
    return S_OK;
}

HRESULT CComBeHavior::Initialize(void* pArg)
{
	auto pDesc = static_cast<BEHAVIOR_DESC*>(pArg);
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;
    
    CBTRoot::BTROOT_DESC BtRoot{};
	m_ComponentName = pDesc->OwnerName;
    BtRoot.Handle    = GetGameObject()->GetHandle();
    BtRoot.NodeName = "Root";
    BtRoot.m_GuiNode = (GUINODE(BEHAVIOR::SELECTOR, m_iNodeID, BtRoot.NodeName.c_str(), _float2(40, 50), 0.5f, _float4(0.5f, 0.5f, 0.5f, 1)));
    BtRoot.m_GuiLink = (GUINODE_LINK(2));

	auto pProto = CGameInstance::Get().ClonePrototype(NODEGROUP::ROOT, "BTRoot", &BtRoot);

	auto proot = Cast<CBTComposite>(pProto.release());
	if (!proot)
		return E_FAIL;
	proot->Set_OwnerName(m_ComponentName);
	m_Root = std::move(ToUPtr(proot));
    m_NodeMap[m_iNodeID++] = m_Root.get();

	m_pBlackBoard = CBTBlackBoard::Create();
	if (nullptr == m_pBlackBoard)
		return E_FAIL;

	if (!pDesc->resBeHaviorMajor.empty() && !pDesc->resBeHaviorMinor.empty())
		Load_DataByResource(pDesc->resBeHaviorMajor, pDesc->resBeHaviorMinor);
	else
	{
		Load_Data(pDesc->LoadPath);
	}

	
	return S_OK;
}
void CComBeHavior::Set_NodeInfo(CBTRoot* pNode, std::vector<uint32_t>& SubTree)
{
    BEHAVIOR eType = pNode->Get_GuiNodeInfo().eMyType;
	pNode->Set_OwnerName(m_ComponentName);
    pNode->Set_Handle(GetGameObject()->GetHandle());
	uint32_t iMax = 0;
	m_iNodeID = std::max(m_iNodeID,pNode->Get_GuiNodeInfo().iID);
    RegistNode(pNode->Get_GuiNodeInfo().iID, pNode);

    if (eType == BEHAVIOR::SECQUNCE || eType == BEHAVIOR::SELECTOR || eType == BEHAVIOR::RAND_SELECTOR)
    {
        auto& pSrc = (*static_cast<CBTComposite*>(pNode)->Get_Nodes());
        for (auto& iter : pSrc)
        {
			if(iter != nullptr)
				Set_NodeInfo(iter.get(),SubTree);
        }
    }
    else if (eType == BEHAVIOR::DECORATOR)
    {
        auto pSrc = static_cast<CBTDecorator*>(pNode)->Get_Child().get();
		if(pSrc != nullptr)
			Set_NodeInfo(pSrc,SubTree);
	}
	else if (eType == BEHAVIOR::ACTION && pNode->GetMasterName() == "BTSubTree")
	{

		SubTree.push_back(pNode->Get_GuiNodeInfo().iID);
	}
}
void CComBeHavior::ResetNode(CBTRoot* pNode)
{
	BEHAVIOR eType = pNode->Get_GuiNodeInfo().eMyType;
	pNode->AbortExecute();
	if (eType == BEHAVIOR::SECQUNCE || eType == BEHAVIOR::SELECTOR || eType == BEHAVIOR::RAND_SELECTOR)
	{
		auto& pSrc = *static_cast<CBTComposite*>(pNode)->Get_Nodes();
		for (auto& iter : pSrc)
		{
			if(iter != nullptr)
				ResetNode(iter.get());
		}
	}
	else if (eType == BEHAVIOR::DECORATOR)
	{
		auto pSrc = static_cast<CBTDecorator*>(pNode);
		if (nullptr != pSrc->Get_Child())
			ResetNode(pSrc->Get_Child().get());
	}
	else if (eType == BEHAVIOR::ACTION && pNode->GetMasterName() == "BTSubTree")
	{
		auto pSrc = static_cast<CBTSubTreeNode*>(pNode);

		if (nullptr != pSrc->Get_SubTreeNode())
			ResetNode(pSrc->Get_SubTreeNode());
	}

}

HRESULT CComBeHavior::Save_Data(const _string& filePath)
{
	if (m_Root->Get_Nodes()->empty() || m_Root->Get_Nodes()->front() == nullptr)
	{
		MSG_BOX("Root Node Not Connected");
		return E_FAIL;
	}

    nlohmann::json j;

    j = m_Root->Save_Node();

    std::ofstream path(filePath);
    path << j.dump(4);
    path.close();

    return S_OK;
}
HRESULT CComBeHavior::Load_Data(const _string& filePath)
{
	m_Root->Get_Nodes()->clear();
	m_iNodeID = 0;
	m_NodeMap.clear();
    nlohmann::json j;
    std::ifstream file(filePath);

    if (!file.is_open())
    {
        MSG_BOX("Node Data Load Failed");
        return E_FAIL;
    }

    file >> j;
    m_Root->Load_json(j);
    file.close();
	
	std::vector<uint32_t> SubTree;
	for(auto& iter : *m_Root->Get_Nodes())
		 Set_NodeInfo(iter.get(),SubTree);
	++m_iNodeID;


	for (auto& iter : SubTree)
	{
		auto pSrc = Find_Node(iter);
		if (nullptr != pSrc)
		{
			auto pNode = static_cast<CBTSubTreeNode*>(pSrc);
			if (nullptr != pNode->Get_SubTreeNode())
				pNode->ConnectNode(pNode->Get_SubTreeNode(),-1,this);
		}
	}
	
    return S_OK;
}
HRESULT CComBeHavior::Load_DataByResource(const _string& restagMajor, const _string& restagMinor)
{
	auto pRes = CGameInstance::Get().GetResourceFirst<CResJson>(restagMajor, restagMinor);
	if (nullptr == pRes)
	{ 
		MSG_BOX("Load Failed Json To BeHavior");
		return E_FAIL;
	}
	m_Root->Load_json(pRes->Get_Json());
	std::vector<uint32_t> SubTree;

	for (auto& iter : *m_Root->Get_Nodes())
		Set_NodeInfo(iter.get(), SubTree);
	++m_iNodeID;

	for (auto& iter : SubTree)
	{
		auto pSrc = Find_Node(iter);
		if (nullptr != pSrc)
		{
			auto pNode = static_cast<CBTSubTreeNode*>(pSrc);
			if (nullptr != pNode->Get_SubTreeNode())
				pNode->ConnectNode(pNode->Get_SubTreeNode(), -1, this);
		}
	}
	return S_OK;
}
CBTRoot* CComBeHavior::Find_Node(const uint32_t& iNode)
{
    auto iter = m_NodeMap.find(iNode);
   
    if (iter != m_NodeMap.end())
        return iter->second;

    return nullptr;
}

void CComBeHavior::Add_Node(CBTRoot* pParent,  uint32_t iSlot, UPtr<CBTRoot> pNode)
{
	if (nullptr == pParent)
		return;

	BEHAVIOR eType = pParent->Get_GuiNodeInfo().eMyType;
	if (nullptr == pNode) return;
	if (eType == BEHAVIOR::SECQUNCE || eType == BEHAVIOR::SELECTOR || eType == BEHAVIOR::RAND_SELECTOR)
	{

		pNode->Set_Handle(GetGameObject()->GetHandle());
		pNode->Set_OwnerName(m_ComponentName);
		RegistNode(pNode->Get_GuiNodeInfo().iID, pNode.get());
		static_cast<CBTComposite*>(pParent)->Add_Node(iSlot,std::move(pNode));
		
	}
	else if (eType == BEHAVIOR::DECORATOR)
	{

		pNode->Set_Handle(GetGameObject()->GetHandle());
		pNode->Set_OwnerName(m_ComponentName);

		RegistNode(pNode->Get_GuiNodeInfo().iID, pNode.get());
		static_cast<CBTDecorator*>(pParent)->Set_Child(std::move(pNode));
	}
}


CBTComposite* CComBeHavior::Get_Selector()
{
    return m_Root.get(); 
}


void CComBeHavior::RegistNode(uint32_t iIndex, CBTRoot* pNode)
{
	auto iter = m_NodeMap.find(iIndex);

	if (iter == m_NodeMap.end())
	{
		//pNode
		m_NodeMap[iIndex] = pNode;
	}
        
}

void CComBeHavior::UnRegistNode(uint32_t iIndex)
{
    auto iter = m_NodeMap.find(iIndex);

	if (iter != m_NodeMap.end())
	{
		BEHAVIOR eType = iter->second->Get_GuiNodeInfo().eMyType;
		if (eType == BEHAVIOR::SECQUNCE || eType == BEHAVIOR::SELECTOR || eType == BEHAVIOR::RAND_SELECTOR)
		{
			auto& pSrc = *static_cast<CBTComposite*>(iter->second)->Get_Nodes();
			for (auto& iter : pSrc)
			{ if(nullptr != iter)
				UnRegistNode(iter->Get_GuiNodeInfo().iID);
			}
		}
		else if (eType == BEHAVIOR::DECORATOR)
		{
			auto& pSrc = static_cast<CBTDecorator*>(iter->second)->Get_Child();
			if (nullptr != pSrc)
				UnRegistNode(pSrc->Get_GuiNodeInfo().iID);
		}


		m_NodeMap.erase(iIndex);
	}
		
}

void CComBeHavior::AbortNode()
{
	if (Check_Flag(ETOUI(CBTRoot::BTFLAG::ABORT)))
	{
		ResetNode(m_Root.get());
		m_iFlag &= ~ETOUI(CBTRoot::BTFLAG::ABORT);
	}
}

_bool CComBeHavior::Check_Flag(uint32_t iFlag)
{
	if (m_iFlag & iFlag)
		return true;

	return false;
}

void CComBeHavior::Set_Flag(uint32_t iFlag, FLAGTYPE eType)
{
	if (eType == FLAGTYPE::ADD)
		m_iFlag |= iFlag;
	else if (eType == FLAGTYPE::DEL)
		m_iFlag &= ~iFlag;
	else if (eType == FLAGTYPE::RESET)
		m_iFlag = iFlag;
	else if (eType == FLAGTYPE::INVERT)
		m_iFlag ^= iFlag;
}

CBTBlackBoard* CComBeHavior::Get_Blackboard()
{
	if(nullptr == m_pBlackBoard)
		return nullptr;

	return m_pBlackBoard.get();
}

void CComBeHavior::Update(_float fTimeDelta)
{
	if (nullptr != m_Root)
	{
		m_Root->ResetDebug();
		m_Root->Tick(fTimeDelta);
	}

}
void CComBeHavior::UpdateGUI()
{
    CComponent::UpdateGUI();

    
    if (ImGui::Button("Open BTEditor"))
    {
        
        CGameInstance::Get().OpenBeHavior(GetGameObject()->GetHandle());
    }
}
UPtr<CComBeHavior> CComBeHavior::Create()
{
    auto pInstance = ToUPtr(new CComBeHavior{});
    if (FAILED(pInstance->InitializePrototype()))
    {
        MSG_BOX("Failed to Created : CComBeHavior");
        return nullptr;
    }
    return pInstance;
}

UPtr<CPrototype> CComBeHavior::Clone(void* pArg)
{
    auto pInstance = ToUPtr(new CComBeHavior{});
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Created : CComBeHavior_Clone");
        return nullptr;
    }
    return pInstance;
}
