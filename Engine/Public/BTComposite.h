#pragma once
#include "BTRoot.h"

//셀렉터 시퀀스용도
NS_BEGIN(Engine)

class  CBTComposite : public CBTRoot
{
public:
	DECLARE_DERIVED_TYPE(CBTComposite, CBTRoot)
protected:
	explicit CBTComposite();
	CBTComposite(const CBTComposite& rhs);
	 ~CBTComposite() override;

	 HRESULT InitializePrototype(void* pArg = nullptr) override { m_MasterName = "Root"; return S_OK; }
	 virtual HRESULT Initalize(void* pArg) override;

	 virtual						void OnEnter() {};
	 virtual						void OnExit(EVALUATE eResult) {};
protected:
	typedef struct strnodevalue
	{
		_string strCurSecquenceName;
		int32_t	iCurSecquenceIndex = { -1 };
		int32_t	iPreSecquenceIndex = { -1 };
		_bool	bCur{ false };
	}NODE_VALUE;
public:
	std::vector<UPtr<CBTRoot>>* Get_Nodes() { return &m_Actions; }
	HRESULT						Save_SubTree(const _string& SavePath) override;
	virtual EVALUATE			Evaluate(_float fTimeDelta) { return EVALUATE::SUCCESS; }
	void						Abort() override;
	void						Tick(_float fTimeDelta);
	void						ResetDebug() override;
public:
	HRESULT						Add_Node(uint32_t iIndex, UPtr<CBTRoot> pNode);
	virtual nlohmann::json		Save_Node() override;
	virtual HRESULT				Load_json(const nlohmann::json& j);
protected:
	NODE_VALUE				m_NodeValue{};

	std::vector<UPtr<CBTRoot>>			  m_Actions;
public:
	static  UPtr<CBTComposite> Create(void* pArg);

	UPtr<CPrototype> Clone(void* pArg) override;
};
NS_END
