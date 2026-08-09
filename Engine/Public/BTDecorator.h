#pragma once
#include "BTRoot.h"

NS_BEGIN(Engine)
class  ENGINE_DLL CBTDecorator : public CBTRoot
{
public:
	DECLARE_DERIVED_TYPE(CBTDecorator, CBTRoot)
	CBTDecorator& operator=(const CBTDecorator&) = delete;
public:
	typedef struct tagdecorator : CBTRoot::BTROOT_DESC
	{

	}DECORATOR_DESC;

protected:
	explicit CBTDecorator();
	CBTDecorator(const CBTDecorator& Prototype);
	~CBTDecorator() override;

	virtual HRESULT	InitializePrototype(void* pArg) override;
	virtual HRESULT Initalize(void* pArg) override;

	virtual void OnEnter() override;
	virtual void OnExit(EVALUATE eResult)override;
public:
	HRESULT				Save_SubTree(const _string& SavePath) override;
	virtual EVALUATE	Evaluate(_float fTimeDelta)override;
	void						Abort() override;
	virtual void				Update_Gui() PURE;
	virtual void				ResetDebug() override;
	nlohmann::json				Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j)override;
	UPtr<CBTRoot>&				Get_Child() { return m_pDecorator; }
	void						Set_Child(UPtr<CBTRoot> pRoot) { 
		if(nullptr == m_pDecorator)
		m_pDecorator = std::move(pRoot); }
private:
	UPtr<CBTRoot>				m_pDecorator{ nullptr };

};

NS_END
