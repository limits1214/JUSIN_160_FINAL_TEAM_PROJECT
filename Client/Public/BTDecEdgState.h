#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"
#include "EnderDragon_State.h"
#include "BlackBoardKey.h"
NS_BEGIN(Client)
class CBTDecEdgState final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecEdgState, CBTDecorator)
private:
	CBTDecEdgState();
	CBTDecEdgState(const CBTDecEdgState& rhs);
	~CBTDecEdgState() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE					Evaluate(_float fTimeDelta) override;
	virtual void				Update_Gui() override;

	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;
private:
	EDG_STATE					m_eState{ EDG_STATE::SPAWN };
public:
	static UPtr<CBTDecEdgState> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

