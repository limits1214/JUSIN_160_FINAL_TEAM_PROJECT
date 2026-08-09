#pragma once
#include "Client_Defines.h"
#include "BTDecorator.h"

enum class TIMER {PAUSE, NEXT, TIMEOUT,TIMEIN_SUCCESS};
NS_BEGIN(Client)
class CBTDecTimer final : public CBTDecorator
{
public:
	DECLARE_DERIVED_TYPE(CBTDecTimer, CBTDecorator)
private:
	CBTDecTimer();
	CBTDecTimer(const CBTDecTimer& rhs);
	~CBTDecTimer() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;
	
	EVALUATE						PAUSE(_float fTimeDelta); //일정 시간동안 대기후 실행
	EVALUATE						NEXT(_float fTimeDelta); //일정 시간동안 바로 실행
	EVALUATE						TimeOut(_float fTimeDelta); //일정 시간안에 성공 못하면 FAILED
	EVALUATE						TimeInSuccess(_float fTimeDelta);//성공시 일정 시간동안 재진입 금지
public:
	EVALUATE						Evaluate(_float fTimeDelta) override;
	void							Abort() override;
	void							Update_Gui() override;

	virtual nlohmann::json			Save_Node()override;

	void		OnEnter() override;
	void		OnExit(EVALUATE eResult) override;
	HRESULT							Load_json(const nlohmann::json& j) override;
private:

	_bool							m_bRun{ true }, m_bFailed{true};
	_float							m_fWaitTime{}, m_fTick{}, m_fAddTime{};
	TIMER							m_eTimer{ TIMER::PAUSE };
public:
	static UPtr<CBTDecTimer> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END

