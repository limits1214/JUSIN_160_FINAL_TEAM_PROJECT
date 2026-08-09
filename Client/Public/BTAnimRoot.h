#pragma once
#include "Client_Defines.h"
#include "BTActionNode.h"
#include "Monster.h"
NS_BEGIN(Client)
typedef struct animflag
{
	_bool bFlag{ false };
	_float fRatio{0};
	FLAGTYPE eType{ FLAGTYPE::RESET};
	uint32_t iFlag{0};
}FLAG_EVENT;
class CBTAnimRoot : public CBTActionNode
{
public:
	DECLARE_DERIVED_TYPE(CBTAnimRoot, CBTActionNode)

protected:
	CBTAnimRoot();

	CBTAnimRoot(const CBTAnimRoot& rhs);
	~CBTAnimRoot() override;
	// CBTActionNode을(를) 통해 상속됨

	HRESULT	InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initalize(void* pArg)override;
public:
	EVALUATE				Evaluate(_float fTimeDelta) override { return EVALUATE::SUCCESS; };
	virtual void			Update_Gui() override;
	void					Abort() override;
	virtual nlohmann::json	Save_Node()override;
	HRESULT					Load_json(const nlohmann::json& j) override;

	_bool				Active_Skill();
protected:
	void				EventFlagToRatio(_float fRatio);
	void				Gravity();
	
protected:
	void				Reset_CheckFlag();

	virtual void		OnEnter() override;
	virtual void		OnExit(EVALUATE eResult) override;
private:
	//GUi
	void				Combo(const _char* pName,uint32_t& iFlag);
	void				Combo2(const _char* pName, FLAGTYPE& eType);
	void				AddSound();
	void				SoundTableValueList();
	void				SoundPopUp(MONSOUND& Sound, CMonster* Monster);
protected:
	void				DragFloat(const _char* pName, _float& fValue);
	void				BoolButton(const _char* pName, _bool& bButton);
	void				Rotation(CComTransform* pTransform, CComCharacterMoveIntent* pMoveIntent, CGameObject* pTarget, _float fTimeDelta, _float fRotRatio);

	void				Play_Sound(_float fTimeDelta);
protected:
	_bool						m_bLoop{ true }, m_bStart{ true }, m_bRatio{ false }, m_bEarly{ false }, m_bGravity{ false }, m_bShow{ false };

	ATTMON						m_eSkillType{ ATTMON::END };
	_float2						m_fSkillRatio{}, m_fRatio{}, m_vRotRatio{};
	_float					     m_fBlend{ 0.1f }, m_fEarlyRatio{ 1.f }, m_fGravity{ -9.8f };
	uint32_t					m_iLoopCnt{ 0 };
	std::vector<FLAG_EVENT>		m_StartFlags{};
	std::vector<FLAG_EVENT>		m_EndFlags{};
	_string						m_strAnimName{};
	
	std::vector<MONSOUND>		m_Sounds{};
private:
	uint32_t					m_iStartFlagCheck{};
	FLAG_EVENT					m_AddFlag{};
public:
	static UPtr<CBTAnimRoot> Create();
	UPtr<CPrototype> Clone(void* pArg)override;
};
NS_END
