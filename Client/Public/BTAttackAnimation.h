#pragma once
#include "Client_Defines.h"
#include "BTAnimRoot.h"

NS_BEGIN(Client)
class CBTAttackAnimation final : public CBTAnimRoot
{
public:
	DECLARE_DERIVED_TYPE(CBTAttackAnimation, CBTAnimRoot)

private:
	CBTAttackAnimation();

	CBTAttackAnimation(const CBTAttackAnimation& rhs);
	~CBTAttackAnimation() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT							InitializePrototype(void* pArg = nullptr) override;
	HRESULT							Initalize(void* pArg)override;
public:
	EVALUATE						Evaluate(_float fTimeDelta) override;
	virtual void					Update_Gui() override;
	void							Abort() override;
	virtual nlohmann::json			Save_Node()override;
	HRESULT							Load_json(const nlohmann::json& j) override;
private:
	void							Att(CMonster* pMon, CComTransform* pSrcTransform, CGameObject* pTarget, _float fRotRatio,_float fTimeDelta);
	void							ShakeCam(_float fRotRatio);

	void OnEnter()override;
	void OnExit(EVALUATE eResult)override;
	_bool ActiveTriggerSkill();
private:
	MOVE				m_eMove{ MOVE::STRAIGHT };

	_float3				m_vEmissiveColor{}, m_vLastPos{}, m_vLastDir{};
	_float2				m_vRatio{}, m_vRotRatio{}, m_vAttRatio{}, m_vOverlabRatio{0.f,0.f};
	_float				m_fDis{}, m_fTime{}, m_fIntensive{ 0.5f }, m_fCamShakeRatio{}, m_fAttRadius{ 5.f }, m_fOverLabSpeed{5.f}, m_fCurOverLabSpeed{};
	_bool				m_bRatioInvert{ false }, m_bActiveSkill{ false }, m_bCamShake{ true }, m_bAttRatio{ false }, m_bOverLabLoop{ false }, m_bOverLabMove{ false }, m_bDir{false};
	_bool				m_bTrigger{ false };
public:
	static UPtr<CBTAttackAnimation> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END
