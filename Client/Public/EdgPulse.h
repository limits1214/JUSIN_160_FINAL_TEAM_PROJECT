#pragma once
#include "DragonSKill.h"


NS_BEGIN(Client)
class CEdgPulse : public CDragonSkill
{
public:
	DECLARE_DERIVED_TYPE(CEdgPulse, CDragonSkill)

protected:
	explicit CEdgPulse();
	explicit CEdgPulse(const CEdgPulse& rhs);
	~CEdgPulse() override;

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
	void			Pulse(_float fTimeDelta);
	_bool			PulseSweep(_vector vNextPos);
public:
	static E::UPtr<CEdgPulse> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

