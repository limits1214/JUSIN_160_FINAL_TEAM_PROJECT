#pragma once
#include "Client_Defines.h"
#include "BTAnimRoot.h"

NS_BEGIN(Client)
class CBTTurnAnimation final : public CBTAnimRoot
{
public:
	DECLARE_DERIVED_TYPE(CBTTurnAnimation, CBTAnimRoot)
private:
	CBTTurnAnimation();

	CBTTurnAnimation(const CBTTurnAnimation& rhs);
	~CBTTurnAnimation() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT						InitializePrototype(void* pArg = nullptr) override;
	HRESULT						Initalize(void* pArg)override;
public:
	EVALUATE					Evaluate(_float fTimeDelta) override;
	virtual void				Update_Gui() override;

	virtual nlohmann::json		Save_Node()override;
	HRESULT						Load_json(const nlohmann::json& j) override;

	void						Abort()override;
private:
	_bool						SelectAngle(_float fAngle);
	void						Turn(_float fTimeDelta);

	virtual void		OnEnter() override;
	virtual void		OnExit(EVALUATE eResult) override;
private:
	_bool						m_bTurn{ false };
	_float						m_fAngle{};
	float						m_fTick{};
	_float3						m_vCurrentLook{}, m_vTargetLook{};
	int32_t						m_iTurnAnimIndex[ETOUI(TURN::END)]{-1}, m_iTurnIdx{-1};
public:
	static UPtr<CBTTurnAnimation> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END
