#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"
#include "EnderDragon_State.h"
#include "EnderDragon.h"
#include "BlackBoardKey.h"
NS_BEGIN(Client)
class CBTDecEdgPhase final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecEdgPhase, CBTDecorator)
private:
	CBTDecEdgPhase();
	CBTDecEdgPhase(const CBTDecEdgPhase& rhs);
	~CBTDecEdgPhase() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE					Evaluate(_float fTimeDelta) override;
	virtual void				Update_Gui() override;

	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;
private:
	DRAGON_PHASE					m_eState{ DRAGON_PHASE::END };
public:
	static UPtr<CBTDecEdgPhase> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

