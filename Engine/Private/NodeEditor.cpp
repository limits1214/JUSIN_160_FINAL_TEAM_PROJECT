#include "pch.h"
#include "NodeEditor.h"
#include "BTSelector.h"
#include "BTSecqunce.h"
#include "BTActionNode.h"
#include "BTDecorator.h"
#include "ComBeHavior.h"
#include "BTSubTreeNode.h"
CNodeEditor::CNodeEditor()
{
}

CNodeEditor::~CNodeEditor()
{
	ax::NodeEditor::DestroyEditor(m_pNodeContext);
}

HRESULT CNodeEditor::Initialize()
{
	
	ax::NodeEditor::Config config;
	config.SettingsFile = nullptr;

	m_pNodeContext = ax::NodeEditor::CreateEditor(&config);

	if (nullptr == m_pNodeContext)
		return E_FAIL;

	Load_FileList();
	return S_OK;
}
void CNodeEditor::UpdateGUI()
{
	
}
void CNodeEditor::RenderGUI()
{
}

void CNodeEditor::NodeEditorUpdate()
{
	if (auto pObj = CGameInstance::Get().GetGameObjectByHandle(m_hTarget))
	{
		if (auto pComBt = pObj->GetComponent<CComBeHavior>("Com_BT"))
		{
			m_pBeHavior = pComBt;
			m_BTNodesMain = pComBt->Get_Selector()->Get_Nodes();

			//ImGui::PushFont(m_FontRegular);
			Show_Editor();
			//ImGui::PopFont();
		}
		else
		{
			m_pBeHavior = nullptr;
			m_BTNodesMain = nullptr;
		}
	}
}

HRESULT CNodeEditor::OpenBeHavior(CHandle Handle)
{
	if (auto pObj = CGameInstance::Get().GetGameObjectByHandle(Handle))
	{
		if (auto pComBt = pObj->GetComponent<CComBeHavior>("Com_BT"))
		{
			m_hTarget = Handle;
			return S_OK;
		}
	}

	//m_BTNodesTmp.clear();
	return E_FAIL;
}


void CNodeEditor::Show_Editor()
{
	if (nullptr == m_pBeHavior) return;
	
	ImGui::Begin("BeHavior Tree");
	// 구해줘
	ImGuiIO& io = ImGui::GetIO();
	_bool	bOpen_Context_Menu = false;//우클릭시 컨텍스트 메뉴롤 열도록 하는거
	int32_t iNode_hovered_in_list = -1; //현재 마우스에 올라간 노드 id hovered된(마우스가 어떤 요소위에 올라가있다) NodeId를 저장하는 변수
	int32_t iNode_hovered_in_scene = -1; //오른쪽 캔버스 공간(실제로 노드를 배치하는곳)에서 노드에 마우스 올라간곳 판정
	const _float fNode_Slot_Radius = 4.0f; //슬롯 반지름
	const _float2 fNode_Window_Padding(8.f, 8.f); //노드 내부 여백

	//왼쪽 패널용도
	NodeList_Panel(&iNode_hovered_in_list,&bOpen_Context_Menu);
	ImGui::BeginGroup(); //위젯 여러개를 묶는용도
	//오른쪽 캔버스창
	Begin_Canvas();
	//화면 격자
	Draw_Grid();

	//링크
	m_pDrawList->ChannelsSplit(2); //Draw List를 두개의 레이어로 나눈다?							   
	
	Draw_Link(); // 0번지에 링크 그리고

	//1번지에 노드 그려서
	Draw_Node(iNode_hovered_in_list, iNode_hovered_in_scene,fNode_Slot_Radius,fNode_Window_Padding,bOpen_Context_Menu, io,m_pBeHavior->Get_Selector());
	for (auto iter = m_BTNodesTmp.begin(); iter != m_BTNodesTmp.end(); ++iter)
	{
		if (Draw_TmpNode(iNode_hovered_in_list, iNode_hovered_in_scene, fNode_Slot_Radius, fNode_Window_Padding, bOpen_Context_Menu, io, (*iter)))
			break;
	}
	////위 2개를 합쳐라
	m_pDrawList->ChannelsMerge();
	

	//context menu 열기전 조건 검사
	//1. 오른쪽 버튼을 뗐다 ISMouseRelease Right
	//2. 현재 창 위에 마우스가 있다 Hovered
	//3. 또는 아무 아이템도 hover도 아니다
	//4. 열어라

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
	{
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) || !ImGui::IsAnyItemHovered())
		{
			if (iNode_hovered_in_list != -1)
				m_iNodeSelect = iNode_hovered_in_list;
			else if (iNode_hovered_in_scene != -1)
				m_iNodeSelect = iNode_hovered_in_scene;
			else
				m_iNodeSelect = -1;
			bOpen_Context_Menu = true;
		}
	}
		
	if (bOpen_Context_Menu)
	{
		ImGui::OpenPopup("context_menu");
		if (iNode_hovered_in_list != -1)
			m_iNodeSelect = iNode_hovered_in_list;

		if (iNode_hovered_in_scene != -1)
			m_iNodeSelect = iNode_hovered_in_scene;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	static _bool bTypeCheck{ false };
	static NODEGROUP eGroupType = NODEGROUP::END;
	if (ImGui::BeginPopup("context_menu"))
	{
		
			auto pNode = m_pBeHavior->Find_Node(m_iNodeSelect);

			if (pNode)
			{

				//노드에서 우클릭 -> Rename delete copy ...
				auto& nodeInfo = pNode->Get_GuiNodeInfo();

				ImGui::Text("Node %s", nodeInfo.Name.c_str());
				ImGui::Separator();

				if (!m_bReName)
				{
					if (ImGui::Selectable("Rename..",false,ImGuiSelectableFlags_DontClosePopups))
					{
						strcpy_s(m_ReName, nodeInfo.Name.c_str()); // 현재 이름으로 초기화
						m_bReName = true;
						ImGui::SetKeyboardFocusHere();	
					}
					if (ImGui::Selectable("CreateSubTree..", false, ImGuiSelectableFlags_DontClosePopups))
					{
						if (pNode->Get_GuiNodeInfo().eMyType != BEHAVIOR::ACTION)
						{
							m_bSubNode = true;
						}
					}

					if (m_bSubNode)
					{
						SaveSubNodePopUp(pNode);
					}
						
				}
				else
				{
					ImGui::Text("Rename node");

					bool submitted = ImGui::InputText(
						"##RenameNode",
						m_ReName,
						IM_ARRAYSIZE(m_ReName),
						ImGuiInputTextFlags_EnterReturnsTrue);

					if (submitted || ImGui::Button("Apply"))
					{
						 nodeInfo.Name = m_ReName;
						 m_bReName = false;
						ImGui::CloseCurrentPopup();
					}

					ImGui::SameLine();

					if (ImGui::Button("Cancel"))
						m_bReName = false;
				}
			}
			else
			{
				if (m_bPopupAction)
					m_bPopupAction = false;
				if (ImGui::MenuItem("Add_Something.."))
				{
					bTypeCheck = true;
					m_bPopupAction = true;
				}

				if (ImGui::MenuItem("Paste", NULL, false, false)) {}
			}



		ImGui::EndPopup();
	}
	ImVec2 vScene_Pos = ImGui::GetMousePosOnOpeningCurrentPopup() - ImVec2(m_vOffset.x, m_vOffset.y);
	if (m_bPopup)
		Add_Node(m_eBTType, m_pNodeName, vScene_Pos);
	if (m_bPopupAction)
	{
			if (bTypeCheck)
			{
				ImGui::OpenPopup("Group_Type");
				if (ImGui::BeginPopup("Group_Type"))
				{
#define X(name)#name,
					const _char* pGroupList[] = { NODE_ACTION_M };
#undef X
					ImGui::Text("Group Name");
					for (uint32_t i = 0; i < ETOUI(NODEGROUP::END); ++i)
					{
						if (ImGui::Button(pGroupList[i]))
						{
							eGroupType = static_cast<NODEGROUP>(i);
							bTypeCheck = false;
						}
					}
					ImGui::EndPopup();
				}

			}
			else if (!bTypeCheck && eGroupType != NODEGROUP::END)
			{
				auto iter = CGameInstance::Get().Show_ActioNode_List(eGroupType, m_pBeHavior->Get_NodeID(), vScene_Pos, m_hTarget);
				if (iter != nullptr)
				{
					Add_NodeToTmp(iter);
					bTypeCheck = true;
					m_bPopupAction = false;
				}
			}
	}
	ImGui::PopStyleVar();

	//휠로 캔버스 이동
	if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f))
	{
		m_vScroll.x += io.MouseDelta.x;
		m_vScroll.y += io.MouseDelta.y;
	}
		
	End_Canvas();
}
void CNodeEditor::NodeList_Panel(int32_t* piNode_hoverd_List, _bool* pbContext_Manu)
{
	ImGui::BeginChild("NodeList", ImVec2(100, 0)); //노드 리스트 ui 왼쪽 패널
	ImGui::Text("Nodes");
	ImGui::Separator();

	if (!m_bSaveLoad && ImGui::Button("Save"))
	{
		m_bSaveLoad = true;
	}
	ImGui::Text("LoadFile list");
	for (auto& iter : m_LoadDataList)
	{
		if (ImGui::Button(iter.first.c_str()))
		{
			//m_BTNodesTmp.clear();
			m_pBeHavior->Load_Data(iter.second);

		}
	}
	
	if (m_bSaveLoad)
	{
		SavePopUp();
	}
	ImGui::EndChild(); ImGui::SameLine();
}
void CNodeEditor::Begin_Canvas()
{
	ImGui::Text("Hold middle mouse button to scroll (%2.f,%2.f)", m_vScroll.x, m_vScroll.y);
	ImGui::SameLine(ImGui::GetWindowWidth() - 100);
	ImGui::Checkbox("Show grid", &m_bShow_grid);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1.f, 1.f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(60, 60, 70, 200));
	ImGui::BeginChild("scrolling_region", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
	ImGui::PopStyleVar();
	ImGui::PushItemWidth(120.f);

	_float2 fCursorScrrenPos = _float2(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);
	XMStoreFloat2(&m_vOffset, XMLoadFloat2(&fCursorScrrenPos) + XMLoadFloat2(&m_vScroll));
	m_pDrawList = ImGui::GetWindowDrawList(); //이거하면 선 원 사각형 직접 그릴수있다네

}
void CNodeEditor::Draw_Grid()
{
	if (m_bShow_grid)
	{ // 격자를 그려라
		ImU32 GRID_COLOR = IM_COL32(200, 200, 200, 40);
		_float GRID_SZ = 64.f;
		ImVec2 vWin_pos = ImGui::GetCursorScreenPos(); //화면 좌표 시작점
		ImVec2 vCanvas_sz = ImGui::GetWindowSize();
		for (_float x = fmodf(m_vScroll.x, GRID_SZ); x < vCanvas_sz.x; x += GRID_SZ)
			m_pDrawList->AddLine(ImVec2(x, 0.f) + vWin_pos, ImVec2(x, vCanvas_sz.y) + vWin_pos, GRID_COLOR);
		for (_float y = fmodf(m_vScroll.y, GRID_SZ); y < vCanvas_sz.y; y += GRID_SZ)
			m_pDrawList->AddLine(ImVec2(0.0f, y) + vWin_pos, ImVec2(vCanvas_sz.x, y) + vWin_pos, GRID_COLOR);
	}
}
void CNodeEditor::Draw_Link()
{
	m_pDrawList->ChannelsSetCurrent(0); // layer0번지에 링크를 그리고
	//아 
	Recursive_Call_Node(m_pBeHavior->Get_Selector());

	for (size_t i = 0; i < m_BTNodesTmp.size(); ++i)
	{
		Recursive_Call_Node((m_BTNodesTmp[i]).get());
	}
	if (nullptr != m_CurrentNode.pCurrentNode)
	{
		Draw_NodeLine(m_pBeHavior->Get_Selector()->GetDebugType(), m_CurrentNode.vSlotPos, _float2(ImGui::GetMousePos().x, ImGui::GetMousePos().y), true);
	}
	if (!ImGui::IsAnyItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		Reset_CurrentNode();
}
void CNodeEditor::Draw_Node(int32_t& iNode_hovered_in_list, int32_t& iNode_hovered_in_scene, const _float& fNode_Slot_Radius, const _float2& fNode_Window_Padding, _bool& bOpen_Context_Menu, ImGuiIO& io,  CBTRoot* pCurNode)
{
	GUINODE* pNode = &pCurNode->Get_GuiNodeInfo();
	GUINODE_LINK* pLink = &pCurNode->Get_GuiNodeLink();

	ImGui::PushID(pNode->iID); // 노드 내부에서 생성되는 widget id 중복방지용
	Widget(pCurNode,pNode, pLink, iNode_hovered_in_list, iNode_hovered_in_scene, fNode_Slot_Radius, fNode_Window_Padding, bOpen_Context_Menu, io);
	int32_t iSlot = Choice_StartSlot(pNode,fNode_Slot_Radius);
	if (-1 != pLink->iStartIdx)
	{ //밀리면 터짐
		if (auto pBeHaivor = m_pBeHavior->Find_Node(pLink->ParentNode.iDestNode))
		{
			if (pBeHaivor->Get_GuiNodeInfo().eMyType == BEHAVIOR::SELECTOR || pBeHaivor->Get_GuiNodeInfo().eMyType == BEHAVIOR::SECQUNCE || pBeHaivor->Get_GuiNodeInfo().eMyType == BEHAVIOR::RAND_SELECTOR)
			{
				if (-1 == ((static_cast<CBTComposite*>(pBeHaivor)->Get_GuiNodeLink().SlotEnd[pLink->iStartIdx].iDestNode)))
				{
					assert(0);
				}

			}
		}
	}

	if (-1 != iSlot)
	{
		GUICURRENT_NODE CurNode(pNode, &pCurNode->Get_GuiNodeLink(), iSlot);
		//그 위에있냐?
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			//시작 기준으로 잡기 
			if (-1 != pLink->iStartIdx && !m_CurrentNode.bSelected )
			{
				//본인과 연결된 부모노드

				if (auto pParentNode = m_pBeHavior->Find_Node(pLink->ParentNode.iDestNode))
				{
					int32_t iParentIndex = pCurNode->Get_GuiNodeLink().iStartIdx;
					pParentNode->Get_GuiNodeLink().SlotEnd[iParentIndex].Reset(); //그 부모도 시작지점 연결 끊기
				
					CurNode.eType = NODETYPE::START;
					CurNode.vSlotPos = pNode->GetStartSlotPos();
					CurNode.bSelected = true;
					CurNode.iD = pNode->iID;
					m_CurrentNode = CurNode;
					if (pParentNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SELECTOR || pParentNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SECQUNCE || pParentNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::RAND_SELECTOR)
					{
						auto& pSrc = (*(static_cast<CBTComposite*>(pParentNode)->Get_Nodes()))[iParentIndex];
						pSrc->Get_GuiNodeLink().iStartIdx = -1;
						pSrc->Get_GuiNodeLink().ParentNode.Reset();
						Add_NodeToTmp(pSrc);
					}
					else if (pParentNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::DECORATOR)
					{
						auto& pSrc = static_cast<CBTDecorator*>(pParentNode)->Get_Child();
						pSrc->Get_GuiNodeLink().iStartIdx = -1;
						pSrc->Get_GuiNodeLink().ParentNode.Reset();
						Add_NodeToTmp(pSrc);
					}

				}
			}
			else if (-1 == pLink->iStartIdx && !m_CurrentNode.bSelected)
			{
				CurNode.eType = NODETYPE::START; //현재 선택한거
				CurNode.vSlotPos = pNode->GetStartSlotPos();
				CurNode.bSelected = true;
				CurNode.iSelectedSlot = iSlot;
				CurNode.iD = pNode->iID;
				m_CurrentNode = CurNode;
			}
		}
	}
	//출력이든 입력이든 해당 노드에서 선택 되는거 판정
	if (pNode->eMyType != BEHAVIOR::ACTION)
	{
		iSlot = Choice_EndSlot(pNode, pLink, fNode_Slot_Radius);
		if (-1 != iSlot)
		{
			GUICURRENT_NODE CurNode(pNode, &pCurNode->Get_GuiNodeLink(), iSlot);
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{//부모노드 기준에서 자식노드로 연결하는거
				//아 이거는 임시 저장소에 넣으면 안되네 이런
				if (-1 != pLink->SlotEnd[iSlot].iDestNode && !m_CurrentNode.bSelected)
				{	//부모기준으로 끊어도 동일하게 연결된 자식이 tmp로 빠지는걸로
					if (pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SELECTOR || pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SECQUNCE || pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::RAND_SELECTOR)
					{
						auto* pNodes = static_cast<CBTComposite*>(pCurNode)->Get_Nodes();
						if (pNodes == nullptr ||
							iSlot < 0 ||
							static_cast<size_t>(iSlot) >= pNodes->size())
						{
							assert(false && "SlotEnd and Actions size mismatch");
							return;
						}

						auto& pSrc = (*pNodes)[iSlot];
						if (pSrc)
						{
							pSrc->Get_GuiNodeLink().iStartIdx = -1;				//자식 기준 부모 끊기
							pSrc->Get_GuiNodeLink().ParentNode.Reset();
							Add_NodeToTmp(pSrc);//지워
						}
						
					}
					else if (pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::DECORATOR)
					{
						auto& pSrc = static_cast<CBTDecorator*>(pCurNode)->Get_Child();
						pSrc->Get_GuiNodeLink().iStartIdx = -1;				//자식 기준 부모 끊기
						pSrc->Get_GuiNodeLink().ParentNode.Reset();
						Add_NodeToTmp(pSrc);//지워
					}
				
					pCurNode->Get_GuiNodeLink().SlotEnd[iSlot].Reset(); // 연결된 자식 끊기
					CurNode.eType = NODETYPE::NODE_END; //현재 선택한거
					CurNode.vSlotPos = pNode->GetEndSlotPos(iSlot,pLink->SlotEnd.size());
					CurNode.bSelected = true;	
					CurNode.iD = pNode->iID;
					m_CurrentNode = CurNode;
					
				}
				else if (-1 == pLink->SlotEnd[iSlot].iDestNode && m_CurrentNode.bSelected && m_CurrentNode.eType == NODETYPE::START)
				{
					for (auto iter = m_BTNodesTmp.begin(); iter != m_BTNodesTmp.end(); ++iter)
					{
						if ((*iter) == nullptr) 
							continue;
						if ((*iter)->Get_GuiNodeInfo().iID == m_CurrentNode.iD)
						{
							(*iter)->Get_GuiNodeLink().iStartIdx = iSlot;
							(*iter)->Get_GuiNodeLink().ParentNode = pCurNode->Get_GuiNodeInfo().Get_DestInfo();
							pCurNode->Get_GuiNodeLink().SlotEnd[iSlot] = (*iter)->Get_GuiNodeInfo().Get_DestInfo();
							m_pBeHavior->Add_Node(pCurNode, iSlot, std::move(*iter));
							Reset_CurrentNode();
							break;
						}
					}
				}
				else if (-1 == pLink->SlotEnd[iSlot].iDestNode && !m_CurrentNode.bSelected)
				{
					CurNode.eType = NODETYPE::NODE_END; //현재 선택한거
					CurNode.vSlotPos = pNode->GetEndSlotPos(iSlot, pLink->SlotEnd.size());
					CurNode.bSelected = true;
					CurNode.iD = pNode->iID;
					m_CurrentNode = CurNode;
				}
			}
			
		}
	}

	if (pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SELECTOR || pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SECQUNCE || pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::RAND_SELECTOR)
	{
		auto pNode = static_cast<CBTComposite*>(pCurNode);
		for (size_t j = 0; j < pNode->Get_Nodes()->size(); ++j)
		{
			if (nullptr == ((*pNode->Get_Nodes())[j]))
				continue;
			
			Draw_Node(iNode_hovered_in_list, iNode_hovered_in_scene, fNode_Slot_Radius, fNode_Window_Padding, bOpen_Context_Menu, io, (*(pNode->Get_Nodes()))[j].get());
		}
	}
	else if (pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::DECORATOR)
	{
		auto pNode = static_cast<CBTDecorator*>(pCurNode);
		if(nullptr != pNode->Get_Child().get())
		Draw_Node(iNode_hovered_in_list, iNode_hovered_in_scene, fNode_Slot_Radius, fNode_Window_Padding, bOpen_Context_Menu, io, pNode->Get_Child().get());

	}
	else if (pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::ACTION && pCurNode->GetMasterName() == "BTSubTree")
	{
		
		auto pNode = static_cast<CBTSubTreeNode*>(pCurNode);
		
		if (pNode->ShowSubTree() && nullptr != pNode->Get_SubTreeNode())
		{
			Draw_Node(iNode_hovered_in_list, iNode_hovered_in_scene, fNode_Slot_Radius, fNode_Window_Padding, bOpen_Context_Menu, io, pNode->Get_SubTreeNode());

		}
			
	}
	ImGui::PopID();


}

_bool CNodeEditor::Draw_TmpNode(int32_t& iNode_hovered_in_list, int32_t& iNode_hovered_in_scene, const _float& fNode_Slot_Radius, const _float2& fNode_Window_Padding, _bool& bOpen_Context_Menu, ImGuiIO& io, UPtr<CBTRoot>& pCurNode)
{
	_bool	bFinishe{ false };
	if ((pCurNode) == nullptr)
		return false;

	GUINODE* pNode = &pCurNode->Get_GuiNodeInfo(); //현재 그리려는 노드를 가져온다
	GUINODE_LINK* pLink = &pCurNode->Get_GuiNodeLink();
	ImGui::PushID(pNode->iID); // 노드 내부에서 생성되는 widget id 중복방지용

	Widget(pCurNode.get(), pNode, pLink, iNode_hovered_in_list, iNode_hovered_in_scene, fNode_Slot_Radius, fNode_Window_Padding, bOpen_Context_Menu, io);
	//동그라미 위에있는지 
	//현재노드의 동글뱅이 거리랑 가깝냐?
	int32_t iSlot = Choice_StartSlot(pNode, fNode_Slot_Radius);
	if (-1 != iSlot)
	{
		GUICURRENT_NODE CurNode(pNode, &pCurNode->Get_GuiNodeLink(), iSlot);
		//그 위에있냐?
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && pCurNode)
		{
			if (!Link_Connect_Check(pCurNode->Get_GuiNodeLink().iStartIdx) && !m_CurrentNode.bSelected) //현재 노드의 Start 부분에 연결된게 있는지?
			{
				CurNode.eType = NODETYPE::START;
				CurNode.vSlotPos = pNode->GetStartSlotPos();
				CurNode.bSelected = true;
				CurNode.iD = pNode->iID;
				m_CurrentNode = CurNode;
				bFinishe = true;
			}
			else if (m_CurrentNode.bSelected && m_CurrentNode.eType == NODETYPE::NODE_END)
			{
				//여기서 원본 클래스 노드 부모에서 tmp 자식 에 연결하기
				auto pSrc = m_pBeHavior->Find_Node(m_CurrentNode.iD);
				if (nullptr != pSrc)
				{

					if (pSrc->Get_GuiNodeInfo().eMyType != BEHAVIOR::ACTION)
					{
						int32_t iPreSlot = m_CurrentNode.iSelectedSlot;
						pCurNode->Get_GuiNodeLink().iStartIdx = iPreSlot; // 자식에 부모 담았고
						pSrc->Get_GuiNodeLink().SlotEnd[iPreSlot] = pCurNode->Get_GuiNodeInfo().Get_DestInfo();
						pCurNode->Get_GuiNodeLink().ParentNode = pSrc->Get_GuiNodeInfo().Get_DestInfo();
						m_pBeHavior->Add_Node(pSrc, iPreSlot, std::move(pCurNode));
					}
				}
				bFinishe = true;
				Reset_CurrentNode();
			}

		}
		else if (m_CurrentNode.bSelected && m_CurrentNode.eType == NODETYPE::START)
		{

		}
	}
	
	ImGui::PopID();
	if (bFinishe) return true;

	if (pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SELECTOR || pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::SECQUNCE || pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::RAND_SELECTOR)
	{
		auto& pNodes = (*static_cast<CBTComposite*>(pCurNode.get())->Get_Nodes());
		for (auto iter = pNodes.begin(); iter != pNodes.end(); ++iter)
		{
			if (Draw_TmpNode(iNode_hovered_in_list, iNode_hovered_in_scene, fNode_Slot_Radius, fNode_Window_Padding, bOpen_Context_Menu, io, (*iter)))
				return true;
		}
	
	}
	else if (pCurNode->Get_GuiNodeInfo().eMyType == BEHAVIOR::DECORATOR)
	{
		auto pNode = static_cast<CBTDecorator*>(pCurNode.get());
		if (Draw_TmpNode(iNode_hovered_in_list, iNode_hovered_in_scene, fNode_Slot_Radius, fNode_Window_Padding, bOpen_Context_Menu, io, pNode->Get_Child()))
			return true;
	}

	return false;
}

int32_t CNodeEditor::Choice_EndSlot(GUINODE* pNode, GUINODE_LINK* pLink, const _float& fNode_Radius)
{
	//해당 노드 선택 됐을 경우에만진입
	for (uint32_t i = 0; i < pLink->SlotEnd.size(); ++i)
	{
		if (ImsMouseHoverSlot(pNode->GetEndSlotPos(i, pLink->SlotEnd.size()), fNode_Radius))
			return i;
	}
	return -1;
}
int32_t CNodeEditor::Choice_StartSlot(GUINODE* pNode, const _float& fNode_Radius)
{
	if (ImsMouseHoverSlot(pNode->GetStartSlotPos(), fNode_Radius))
			return 0;
	
	return -1;
}

void CNodeEditor::End_Canvas()
{//살려줘
	ImGui::PopItemWidth();
	ImGui::EndChild();
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	ImGui::EndGroup();

	ImGui::End();
}

void CNodeEditor::Load_FileList()
{
	_string Path = "./Resources/json/Behavior/";
	m_LoadDataList.clear();
	for (auto& iter : std::filesystem::directory_iterator(Path))
	{
		m_LoadDataList.emplace(iter.path().filename().string(), iter.path().string());
	}
}

void CNodeEditor::DragAllMove(CBTRoot* pRoot, _float2 vPos)
{
	
	BEHAVIOR eType = pRoot->Get_GuiNodeInfo().eMyType;
	_float2 vCurrent = pRoot->Get_GuiNodeInfo().vPos;
	pRoot->Get_GuiNodeInfo().vPos = _float2(vCurrent.x + vPos.x, vCurrent.y + vPos.y);
	
	if (eType == BEHAVIOR::SELECTOR || eType == BEHAVIOR::SECQUNCE || eType == BEHAVIOR::RAND_SELECTOR)
	{
		auto& pSrc = (*static_cast<CBTComposite*>(pRoot)->Get_Nodes());
		if (pSrc.empty())
			return;
		for (size_t i =0; i<  pSrc.size(); ++i)
		{
			if (pSrc[i] != nullptr)
			{
				DragAllMove(pSrc[i].get(), vPos);
			}
		}
	}
	else if (eType == BEHAVIOR::DECORATOR)
	{
		auto& pSrc = (static_cast<CBTDecorator*>(pRoot)->Get_Child());
		if (nullptr == pSrc)
			return;
		
		DragAllMove(pSrc.get(), vPos);
	}
}

void CNodeEditor::SavePopUp()
{

	_string     PathName = "./Resources/json/Behavior/";
	_char		NameBuffer[64]{};
	_char		NameSrc[64]{};
	_float2		ViewPortSize = CGameInstance::Get().GetClientScreenSize();
	ImGui::SetNextWindowPos(
		ImVec2(ViewPortSize.x *0.5f,ViewPortSize.y *0.5f),
		ImGuiCond_Appearing,
		ImVec2(0.5f, 0.5f) // pivot (중앙 기준)
	);
	ImGui::OpenPopup("File Save");

	if (ImGui::BeginPopup("File Save", ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("FileName");
		if (ImGui::InputText("##FileName", &NameBuffer[0], IM_ARRAYSIZE(NameBuffer))) //이름 입력
		{
			m_AddNodeName = PathName + NameBuffer + (".json");
			m_SaveName = NameBuffer;
		}

		
		if (ImGui::Button("Ok"))
		{
			_bool bfalse{false};
			for (auto& iter : std::filesystem::recursive_directory_iterator(PathName))
			{
				if (iter.path().filename().stem() == "")
				{
					bfalse = true;
					MSG_BOX("File Name Blink");
				}
			}
		
			if (!bfalse)
			{
				m_bSaveLoad = false;
				if (FAILED(m_pBeHavior->Save_Data(m_AddNodeName)))
				{

				}else MSG_BOX("Successed Save");

				Load_FileList();
				ImGui::CloseCurrentPopup();
			}
		} ImGui::SameLine(100.f);
		if (ImGui::Button("Cancle"))
		{
			m_bSaveLoad = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void CNodeEditor::SaveSubNodePopUp(CBTRoot* pRoot)
{
	if (nullptr == pRoot)
		return;
	
	_string     PathName = "./Resources/json/Behavior/SubTree/";
	_char		NameBuffer[64]{};
	_char		NameSrc[64]{};
	_float2		ViewPortSize = CGameInstance::Get().GetClientScreenSize();
	ImGui::SetNextWindowPos(
		ImVec2(ViewPortSize.x * 0.5f, ViewPortSize.y * 0.5f),
		ImGuiCond_Appearing,
		ImVec2(0.5f, 0.5f) // pivot (중앙 기준)
	);
	ImGui::OpenPopup("File Save SubTree");

	if (ImGui::BeginPopup("File Save SubTree", ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("FileName");
		if (ImGui::InputText("##FileName", &NameBuffer[0], IM_ARRAYSIZE(NameBuffer))) //이름 입력
		{
			m_AddNodeName = PathName + NameBuffer + (".json");
			m_SaveName = NameBuffer;
		}

		if (ImGui::Button("Ok"))
		{
			
			_bool bfalse{ false };
			for (auto& iter : std::filesystem::recursive_directory_iterator(PathName))
			{
				if (iter.path().filename().stem() == "")
				{
					bfalse = true;
					MSG_BOX("File Name Blink");
				}
			}

			if (!bfalse)
			{
				m_bSubNode = false;
				if (FAILED(pRoot->Save_SubTree(m_AddNodeName)))
				{

				}
				else MSG_BOX("Successed Save");
				ImGui::CloseCurrentPopup();
			}
		} ImGui::SameLine(100.f);
		if (ImGui::Button("Cancle"))
		{
			m_bSubNode = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void CNodeEditor::Widget(CBTRoot* pRoot, GUINODE* pNode, GUINODE_LINK* pLink, int32_t& iNode_hovered_in_list, int32_t& iNode_hovered_in_scene, const _float& fNode_Slot_Radius, const _float2& fNode_Window_Padding, _bool& bOpen_Context_Menu, ImGuiIO& io)
{
	_float2 vMin{};
	XMStoreFloat2(&vMin, XMLoadFloat2(&m_vOffset) + XMLoadFloat2(&pNode->vPos)); // 캔버스 기준 좌표를 스크린 좌표 기준으로 변환
	ImVec2 vNode_Rect_Min(vMin.x, vMin.y);

	m_pDrawList->ChannelsSetCurrent(1); //레이어 1에 UI 그려
	_bool	bOld_Any_Active = ImGui::IsAnyItemActive(); //위젯 생성전에 다른 위젯이 이미 활성화 되어있는지 저장
	ImGui::SetCursorScreenPos(vNode_Rect_Min + ImVec2(fNode_Window_Padding.x, fNode_Window_Padding.y)); //여기에 위젯을 만들어라
	//노드 내부으 imgui 위젯 생성	

	ImGui::BeginGroup();
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.f, 0.f, 0.f, 1.f));
	
	ImGui::Text("%s", pNode->Name.c_str()); //이름..
	ImGui::PopStyleColor();
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.245, 0.6834f, 1.f));
	ShowWidgetByType(pRoot);
	ImGui::PopStyleColor();
	ImGui::SliderFloat("##value", &pNode->fValue, 0.f, 1.f, "Alpha %2.f"); //알파값 뭐 쓸모가..
	ImGui::ColorEdit3("##Color", &pNode->vColor.x); //색상 뭐 쓸모가..

	
	ImGui::EndGroup();
	///////////////////위젯 생성

	_bool bNode_Widgets_active = (!bOld_Any_Active && ImGui::IsAnyItemActive());
	_float2 vDouble_Padding{};
	XMStoreFloat2(&vDouble_Padding, XMLoadFloat2(&fNode_Window_Padding) + XMLoadFloat2(&fNode_Window_Padding));
	ImVec2 vSize = ImGui::GetItemRectSize() + ImVec2(vDouble_Padding.x, vDouble_Padding.y);
	//Group 내부 Widget들의 크기를 자동 계산
	//GetItemRectSize는 gui가 text slider를 배치 후 가로 세로를 자동으로 계산해서 늘려준다네
	pNode->vSize = _float2(vSize.x, vSize.y);

	ImVec2 vNode_Rect_Max = vNode_Rect_Min + vSize;

	//뭐 노드 박스?
	m_pDrawList->ChannelsSetCurrent(0);
	ImGui::SetCursorScreenPos(vNode_Rect_Min);
	ImGui::InvisibleButton("node", vSize); //보이지않는 버튼 Talon
	//노드 전체에 버튼을 깔아서 노드 움직일수 있게해줌
	if (ImGui::IsItemHovered())
	{//마우스가 위에있고 우클릭을 했다? 그럼 매뉴창을연다
		iNode_hovered_in_scene = pNode->iID;
		bOpen_Context_Menu |= ImGui::IsMouseClicked(1);
	}

	_bool bNode_Moving_Active = ImGui::IsItemActive(); //클릭중이냐?
	if (bNode_Widgets_active || bNode_Moving_Active)
		m_iNodeSelect = pNode->iID;
	if (bNode_Moving_Active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) //드래그중이냐?
	{
		
		_float2 Offset = _float2(io.MouseDelta.x, io.MouseDelta.y);
		if (CGameInstance::Get().KeyPressing(DIK_Z) && pNode->eMyType != BEHAVIOR::ACTION)
		{
			DragAllMove(pRoot, Offset);
		}
		else {
			_float2 vPos = _float2(pNode->vPos.x + io.MouseDelta.x, pNode->vPos.y + io.MouseDelta.y);
			pNode->vPos = vPos;
		}
	}
		

	//선택하거나 마우스 위에 올렸을때 scene인가 hover 확인해서 색상으로 표기
	ImU32 Node_Bg_Color = ImGui::ColorConvertFloat4ToU32(ImVec4(pNode->vColor.x, pNode->vColor.y, pNode->vColor.z, pNode->vColor.w));
	//노드 의 외형을 직접 그리는거
	//배경
	m_pDrawList->AddRectFilled(vNode_Rect_Min, vNode_Rect_Max, Node_Bg_Color, 4.f);
	//테두리차두리두리두리두리
	m_pDrawList->AddRect(vNode_Rect_Min, vNode_Rect_Max, IM_COL32(0, 0, 0, 125), 4.f);
	//동글뱅이 입력노드 출력노드
	
	m_pDrawList->AddCircleFilled(ImVec2(m_vOffset.x + pNode->GetStartSlotPos().x, m_vOffset.y + pNode->GetStartSlotPos().y), fNode_Slot_Radius, IM_COL32(255, 50, 50, 255));
	
	if (!pLink->SlotEnd.empty())
	{
		for (uint32_t i = 0; i < pLink->SlotEnd.size(); ++i)
			m_pDrawList->AddCircleFilled(ImVec2(m_vOffset.x + pNode->GetEndSlotPos(i,pLink->SlotEnd.size()).x, m_vOffset.y + pNode->GetEndSlotPos(i, pLink->SlotEnd.size()).y), fNode_Slot_Radius, IM_COL32(255, 50, 50, 255));
	}

}

void CNodeEditor::ShowWidgetByType(CBTRoot* pNode)
{
	BEHAVIOR eNodeType = pNode->Get_GuiNodeInfo().eMyType;

	if (eNodeType == BEHAVIOR::ACTION || eNodeType == BEHAVIOR::DECORATOR)
		CGameInstance::Get().Show_Action_NodeWidget(pNode);
	else if (eNodeType == BEHAVIOR::SELECTOR || eNodeType == BEHAVIOR::SECQUNCE || eNodeType == BEHAVIOR::RAND_SELECTOR)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 1.f));
		if (ImGui::Button("Add Pin"))Pin(pNode, true);
		if (ImGui::Button("Del pin"))Pin(pNode, false);
		ImGui::PopStyleColor();
	}
}

void CNodeEditor::Add_Node(BEHAVIOR eType, const _char* pPopupName, ImVec2 vPos)
{
	CBTRoot::BTROOT_DESC SequenceDesc;
	_string PopupID = _string(pPopupName) + " Popup";
	_string InputName = _string(pPopupName) + " Name :";
	_char Name[32]{};
	_char NameBuffer[32]{};
	UPtr<CBTRoot>  pNode{ nullptr };
	ImGui::OpenPopup(PopupID.c_str());

	if (ImGui::BeginPopup(PopupID.c_str(), ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text(InputName.c_str());
		if (ImGui::InputText("##NodeName", NameBuffer, IM_ARRAYSIZE(NameBuffer))) //이름 입력
			m_AddNodeName = _string(pPopupName) + " " + NameBuffer;
		
		if (ImGui::Button("Add"))
		{
			int32_t iIndex = 0;
			m_bPopup = false;
			_float4 vCor = eType == BEHAVIOR::SECQUNCE ? _float4(0.1f, 1.f, 0.1f,1.f) : _float4(0.5294f, 0.9843f, 1.f, 1.f);
			SequenceDesc.m_GuiNode = GUINODE(eType, m_pBeHavior->Get_NodeID()++, m_AddNodeName.c_str(), _float2(vPos.x, vPos.y), 0.5f, vCor);
			SequenceDesc.m_GuiLink = (GUINODE_LINK(2));
			SequenceDesc.Handle = m_hTarget;
			if (auto iter = m_pBeHavior->Find_Node(m_pBeHavior->Get_NodeID()))
			{
				--m_pBeHavior->Get_NodeID();
				MSG_BOX("Failed : Node Index Problem");
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return;
			}

			;

			if (eType == BEHAVIOR::SELECTOR)
				pNode = engine_uptr_cast<CBTRoot>(CGameInstance::Get().ClonePrototype(NODEGROUP::SELECTOR,"BTSelector",&SequenceDesc));
			else if(eType == BEHAVIOR::SECQUNCE)
				pNode = engine_uptr_cast<CBTRoot>(CGameInstance::Get().ClonePrototype(NODEGROUP::SEQUENCE, "BTSequnce", &SequenceDesc));
			else if (eType == BEHAVIOR::RAND_SELECTOR)
				pNode = engine_uptr_cast<CBTRoot>(CGameInstance::Get().ClonePrototype(NODEGROUP::RAND_SELECTOR, "BTRandSelector", &SequenceDesc));
			else if (eType == BEHAVIOR::SELECTOR)
				pNode = engine_uptr_cast<CBTRoot>(CGameInstance::Get().ClonePrototype(NODEGROUP::SELECTOR, "BTReactiveSelector", &SequenceDesc));

			if (nullptr == pNode)
			{
				ImGui::CloseCurrentPopup();
				ImGui::EndPopup();
				return;
			}
			Add_NodeToTmp(pNode);
			ImGui::CloseCurrentPopup();
		}
		
		if (ImGui::Button("Cancle"))
		{
			m_bPopup = false;
			ImGui::CloseCurrentPopup();
		}
			
		ImGui::EndPopup();
	}

}

_bool CNodeEditor::Link_Connect_Check(int32_t iSlot)
{
	//현재 노드 링크에 담겨있는거 start기준(노드의 id)으로 end랑 연결되게 되어있고
	//이거 nodelink 기준으로 스타트에 담겨있는거를 다시 누르려고하면 떨어지게 해야댐
	//만약 노드의 id가 nodeslink에 안담겨 있으면 해당 노드의 스타트 지점으로 부터 나가는 선은 없음 //이경우는 false지
	//근데 end 지점은또 연결 되어있을수도 있음
	if ( - 1 != iSlot)
		return true;
	return false;
}



void CNodeEditor::Reset_CurrentNode()
{
	m_CurrentNode.eType = NODETYPE::END;
	m_CurrentNode.pCurrentNode = nullptr;
	m_CurrentNode.pCurrentLink = nullptr;
	m_CurrentNode.vSlotPos = _float2(0,0);
	m_CurrentNode.iSelectedSlot = -1;
	m_CurrentNode.bSelected = false;
}
void CNodeEditor::Draw_NodeLine(EVALUATE eType, _float2 iStartnode, _float2 iEndNode, _bool bMouse)
{
	_float2 p1{}, p2{}, input{}, output{};
	input = iStartnode; //동글뱅이 좌표
	output = iEndNode;
	XMStoreFloat2(&p1, XMLoadFloat2(&m_vOffset) +   //캔버스 화면 좌표에 내 좌표 더해서 화면 좌표로
		XMLoadFloat2(&input));

	if (bMouse)
		p2 = iEndNode;
	else
	{
		XMStoreFloat2(&p2, XMLoadFloat2(&m_vOffset) +
			XMLoadFloat2(&output));
	}
	//선 그리라고
	_float4 vColor{};
	switch (eType)
	{
	case EVALUATE::SUCCESS:
		vColor = _float4(0, 255, 0, 255);
		break;
	case EVALUATE::RUN:

		vColor = _float4(0, 0, 255, 255);
		break;

	case EVALUATE::FAILED:
		vColor = _float4(255, 0, 0, 255);
		break;
	default :
		vColor = _float4(0, 0, 0, 255);
		break;
	}
	m_pDrawList->AddBezierCubic(ImVec2(p1.x, p1.y), ImVec2(p1.x, p1.y) + ImVec2(+50, 0),
		ImVec2(p2.x, p2.y) + ImVec2(-50, 0), ImVec2(p2.x, p2.y), IM_COL32(vColor.x, vColor.y, vColor.z, vColor.w), 3.f);
}
_bool CNodeEditor::ImsMouseHoverSlot(_float2 vSlotPos, const _float& fNode_Radius)
{
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 vMousePos = io.MousePos;
	_vector vScreen{}, vMouse{ vMousePos.x, vMousePos .y,0,1.f};
	vScreen = XMLoadFloat2(&m_vOffset) + XMLoadFloat2(&vSlotPos);
	
	_float fDist = XMVectorGetX(XMVector2LengthSq(vScreen - vMouse));
	_bool  bhovered = fDist <= (fNode_Radius +3.f) * (fNode_Radius +3.f);
	return bhovered;
}

void CNodeEditor::Recursive_Call_Node(class CBTRoot* pParent)
{
	if (nullptr == pParent)
		return;
	GUINODE_LINK* pLink = &pParent->Get_GuiNodeLink();
	GUINODE* pNode_inp{ nullptr };
	GUINODE* pNode_out{ nullptr };
	BEHAVIOR eType = pParent->Get_GuiNodeInfo().eMyType;
	if (BEHAVIOR::SELECTOR == eType || BEHAVIOR::SECQUNCE == eType || BEHAVIOR::RAND_SELECTOR == eType)
	{
		auto pNode = static_cast<CBTComposite*>(pParent);
		auto& pNodeArray = (*pNode->Get_Nodes());

		for (size_t i = 0; i < pNodeArray.size(); ++i)
		{
			if(nullptr == pNodeArray[i])
				continue;
			int32_t iParentIndex = pNodeArray[i]->Get_GuiNodeLink().iStartIdx;

			pNode_inp = &pNodeArray[i]->Get_GuiNodeInfo();
			pNode_out = &pParent->Get_GuiNodeInfo();
			Draw_NodeLine(pNodeArray[i]->GetDebugType(),pNode_inp->GetStartSlotPos(), pNode_out->GetEndSlotPos(iParentIndex, pParent->Get_GuiNodeLink().SlotEnd.size()));


			if ((*pNode->Get_Nodes())[i]->Get_GuiNodeInfo().eMyType != BEHAVIOR::ACTION)
				Recursive_Call_Node(pNodeArray[i].get());
			else if (BEHAVIOR::ACTION == (*pNode->Get_Nodes())[i]->Get_GuiNodeInfo().eMyType &&
				(*pNode->Get_Nodes())[i]->GetMasterName() == "BTSubTree")
			{
				auto pSubTree = static_cast<CBTSubTreeNode*>(pNodeArray[i].get());
				if(nullptr != pSubTree && pSubTree->ShowSubTree())
				Recursive_Call_Node(pSubTree->Get_SubTreeNode());
			}
				
		}
	}
	else if (BEHAVIOR::DECORATOR == eType)
	{
		auto pNode = static_cast<CBTDecorator*>(pParent);
		auto& pChild = pNode->Get_Child();
		if (nullptr != pChild)
		{
			int32_t iParentIndex = pChild->Get_GuiNodeLink().iStartIdx;

			pNode_inp = &pChild->Get_GuiNodeInfo();
			pNode_out = &pParent->Get_GuiNodeInfo();
			Draw_NodeLine(pChild->GetDebugType(),pNode_inp->GetStartSlotPos(), pNode_out->GetEndSlotPos(iParentIndex, pParent->Get_GuiNodeLink().SlotEnd.size()));

			if ((pNode->Get_GuiNodeInfo().eMyType != BEHAVIOR::ACTION) )
				Recursive_Call_Node(pChild.get());
			else if (pChild->Get_GuiNodeInfo().eMyType == BEHAVIOR::ACTION && pChild->GetMasterName() == "BTSubTree")
			{
				auto pSub = static_cast<CBTSubTreeNode*>(pChild.get());
				
				if(nullptr!= pSub && pSub->ShowSubTree())
					Recursive_Call_Node(pSub->Get_SubTreeNode());
			}
				
		}
		
	}
	else if (BEHAVIOR::ACTION == eType && pParent->GetMasterName() == "BTSubTree")
	{
		auto pNode = static_cast<CBTSubTreeNode*>(pParent);
		if (nullptr != pNode && pNode->ShowSubTree() && nullptr != pNode->Get_SubTreeNode())
		{
			Recursive_Call_Node(pNode->Get_SubTreeNode());
		}
			
	}
	
}

void CNodeEditor::Pin(CBTRoot* pNode, _bool bPin)
{
	if (bPin)
	{
		pNode->Get_GuiNodeLink().SlotEnd.push_back(DEST_NODE());
		static_cast<CBTComposite*>(pNode)->Get_Nodes()->push_back(nullptr);
	}
	else
	{
		if (pNode->Get_GuiNodeLink().SlotEnd.empty())
			return;

		DEST_NODE pDest = pNode->Get_GuiNodeLink().SlotEnd.back();
		int32_t   iSlot = pNode->Get_GuiNodeLink().SlotEnd.size() - 1;
		if (-1 != pDest.iDestNode)
		{
			auto& pDestNode = (*(static_cast<CBTComposite*>(pNode)->Get_Nodes()))[iSlot];
				pDestNode->Get_GuiNodeLink().iStartIdx = -1;
			if (auto iter = m_pBeHavior->Find_Node(pDest.iDestNode))
				Add_NodeToTmp(pDestNode);
		}
		static_cast<CBTComposite*>(pNode)->Get_Nodes()->pop_back();
		pNode->Get_GuiNodeLink().SlotEnd.pop_back();
	}
}

void CNodeEditor::Add_NodeToTmp(UPtr<class CBTRoot>& pRoot)
{
	if (nullptr == pRoot)
	{
		MSG_BOX("Add Failed To Tmp Node");
		return;
	}
	m_pBeHavior->UnRegistNode(pRoot->Get_GuiNodeInfo().iID);

	if (m_BTNodesTmp.empty())
		m_BTNodesTmp.push_back(std::move(pRoot));
	else
		for (size_t i = 0; i < m_BTNodesTmp.size(); ++i)
		{
			if (nullptr == m_BTNodesTmp[i])
			{
				m_BTNodesTmp[i] = std::move(pRoot);
				break;
			}
			else
			{
				m_BTNodesTmp.push_back(std::move(pRoot));
				break;
			}
		}
}

UPtr<CNodeEditor> CNodeEditor::Create()
{
	auto pInstance = ToUPtr(new CNodeEditor());
	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Create Failed : NodeEditor");
		return nullptr;
	}
	return pInstance;

}
