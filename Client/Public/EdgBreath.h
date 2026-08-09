#pragma once
#include "DragonSKill.h"


NS_BEGIN(Client)
class CEdgBreath : public CDragonSkill
{
public:
	DECLARE_DERIVED_TYPE(CEdgBreath, CDragonSkill)

protected:
	explicit CEdgBreath();
	explicit CEdgBreath(const CEdgBreath& rhs);
	~CEdgBreath() override;

public:
	HRESULT			InitializePrototype(void* pArg = nullptr) override;
	HRESULT			Initialize(void* pArg) override;
	void			PriorityUpdate(E::_float fTimeDelta) override;
	void			FixedUpdate(E::_float fTimeDelta) override;
	void			Update(E::_float fTimeDelta) override;
	void			LateUpdate(E::_float fTimeDelta) override;
public:
	void			Active(const _string& SkillName) override;
	void			Cancle() override;
private:
	void			MoveBreath(_float fTimeDelta);
	_bool			MoveSweep(_vector vNextPos,_vector vCurDir);
private:
	_float			m_fBreathTick{};
public:
	static E::UPtr<CEdgBreath> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

