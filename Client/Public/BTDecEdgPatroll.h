#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"
#include "EnderDragon_State.h"
#include "EnderDragon.h"
#include "BlackBoardKey.h"
NS_BEGIN(Client)
class CBTDecEdgPatroll final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecEdgPatroll, CBTDecorator)
private:
	CBTDecEdgPatroll();
	CBTDecEdgPatroll(const CBTDecEdgPatroll& rhs);
	~CBTDecEdgPatroll() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
public:
	EVALUATE					Evaluate(_float fTimeDelta) override;
	virtual void				Update_Gui() override;

	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;
private:
	EVALUATE						Patroll(MOVE eState, CBTBlackBoard* pBB);
	EVALUATE							Moving(_float3& vOutDir, _float3 vSrcPos,  const StringID ArrowKey, CBTBlackBoard* pBB);
private:
	MOVE							m_eState{};
public:
	static UPtr<CBTDecEdgPatroll> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

