#pragma once
#include "GameObject.h"

NS_BEGIN(Engine)
//using namespace ax;

class CNodeEditor : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CNodeEditor, CEngineBase)
protected:
	explicit CNodeEditor();
	~CNodeEditor()override;

private:
	HRESULT	   Initialize();
public:
	void	   UpdateGUI();
	void	   RenderGUI();
	void	   NodeEditorUpdate();

	HRESULT	   OpenBeHavior(CHandle Handle);
private:
	void	   Show_Editor();
	void	   NodeList_Panel(int32_t* piNode_hoverd_List, _bool* pbContext_Manu);
	void	   Begin_Canvas();

	void	   Draw_Grid();
	void	   Draw_Link();
	
	void	   Draw_Node(int32_t& iNode_hovered_in_list, int32_t& iNode_hovered_in_scene, const _float& fNode_Slot_Radius, const _float2& fNode_Window_Padding, _bool& bOpen_Context_Menu, ImGuiIO& io, class CBTRoot* pCurNode);
	_bool	   Draw_TmpNode(int32_t& iNode_hovered_in_list, int32_t& iNode_hovered_in_scene, const _float& fNode_Slot_Radius, const _float2& fNode_Window_Padding, _bool& bOpen_Context_Menu, ImGuiIO& io, UPtr<class CBTRoot>& pCurNode);
	int32_t    Choice_EndSlot(GUINODE* pNode,   GUINODE_LINK* pLink, const _float& fNode_Radius);
	int32_t    Choice_StartSlot(GUINODE* pNode,  const _float& fNode_Radius);

	void	   End_Canvas();


private:
	//기능
	void		Load_FileList();
	void		DragAllMove(CBTRoot* pRoot ,_float2 vPos);
	void		SavePopUp();
	void        SaveSubNodePopUp(CBTRoot* pRoot);
	void		Widget(CBTRoot* pRoot , GUINODE* pNode, GUINODE_LINK* pLink,int32_t& iNode_hovered_in_list, int32_t& iNode_hovered_in_scene, const _float& fNode_Slot_Radius, const _float2& fNode_Window_Padding, _bool& bOpen_Context_Menu, ImGuiIO& io);
	void		ShowWidgetByType(CBTRoot* pRoot);
	void		Add_Node(BEHAVIOR eType, const _char* pPopupName, ImVec2 vPos);
	_bool		Link_Connect_Check(int32_t iSlot);
	void		Reset_CurrentNode();
	void		Draw_NodeLine(EVALUATE eType, _float2 iStartnode, _float2 iEndNode,_bool bMouse = false);
	_bool		ImsMouseHoverSlot(_float2 vSlotPos, const _float& fNode_Radius);
	void		Recursive_Call_Node(class CBTRoot* pParent);
	void		Pin(CBTRoot* pNode,_bool bPin);
	void		Add_NodeToTmp(UPtr<class CBTRoot>& pRoot);

private:
	ax::NodeEditor::EditorContext*						m_pNodeContext{ nullptr };
	std::vector<GUINODE>								m_Nodes;
	std::vector<GUINODE_LINK>							m_NodesLink;
	
	std::vector<UPtr<class CBTRoot>>*					m_BTNodesMain{ nullptr };
	std::vector<UPtr<class CBTRoot>>					m_BTNodesTmp;


	class CComBeHavior*									m_pBeHavior{ nullptr };
	ImDrawList*											m_pDrawList{ nullptr };
	GUICURRENT_NODE										m_CurrentNode{};

	_float2												m_vScroll{ 0,0 }, m_vOffset{};
	_bool												m_binited{ false }, m_bShow_grid{ true }, m_bPopup{ false }, m_bPopupAction{ false }, m_bSaveLoad{ false }, m_bReName{ false }, m_bSubNode{ false };
														
	int32_t												m_iNodeSelect{ -1 }, iNodeID{0};
														
	_string												m_AddNodeName{};
														
	CHandle												m_hTarget{};

	_string												m_SaveName{};
	const _char*										m_pNodeName{ nullptr };
	char												m_ReName[128]{};
	BEHAVIOR											m_eBTType{ BEHAVIOR::END };
	ImFont*												m_FontRegular{ nullptr };
												
	std::map<_string, _string>							m_LoadDataList;
public:
	static UPtr<CNodeEditor> Create();
	
	
};

NS_END

