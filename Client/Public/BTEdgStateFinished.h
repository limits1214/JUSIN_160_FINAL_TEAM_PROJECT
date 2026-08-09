#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"

NS_BEGIN(Client)
class CBTEdgStateFinished final : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTEdgStateFinished, CBTActionNode)
private:
	CBTEdgStateFinished();
	CBTEdgStateFinished(const CBTEdgStateFinished& rhs);
	~CBTEdgStateFinished() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE Evaluate(_float fTimeDelta) override;
	virtual void		Update_Gui() override;

	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;

	void OnEnter()override;
	void OnExit(EVALUATE eResult)override;
public:
	static UPtr<CBTEdgStateFinished> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

