#pragma once
#include "BTActionNode.h"

NS_BEGIN(Engine)

class ENGINE_DLL CBTSubTreeNode : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTSubTreeNode, CBTActionNode)
	CBTSubTreeNode& operator=(const CBTSubTreeNode&) = delete;

public:


protected:
	explicit CBTSubTreeNode();
	CBTSubTreeNode(const CBTSubTreeNode& pPrototype);
	~CBTSubTreeNode() override;

	HRESULT					InitializePrototype(void* pArg) override;
	HRESULT					Initalize(void* pArg) override;


public:
	void						Abort() override;
	virtual void				Update_Gui();
	ACTION_VALUE& Get_Value() { return m_Value; }
public:
	void						CreateSubTree();
	virtual EVALUATE			Evaluate(_float fTimeDelta)override;
	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;

	void						OnEnter() override;
	void						OnExit(EVALUATE eResult)override;
	void						ResetDebug()override;
	CBTRoot*					Get_SubTreeNode();
	_bool						ShowSubTree() { return m_bIsSubTree; }
	void						ConnectNode(CBTRoot* pNode,int32_t iPreIndex , class CComBeHavior* pBehavior = nullptr,CBTRoot* pParent = nullptr);

private:
	void						InputPopUp(const _string& strPopupName, _string& strTagName,_bool& bPopUp);
private:
	_string		m_strResMajor{}, m_strResMinor{};
	UPtr<class CBTRoot>		m_pSubTreeRoot{};
	_bool							m_bIsSubTree{ false }, m_bMajorPop{}, m_bMinorPop{};
public:
	static UPtr<CBTSubTreeNode> Create(void* pArg = nullptr);
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
