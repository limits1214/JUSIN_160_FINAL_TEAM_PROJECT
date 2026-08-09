#pragma once
#include "DragonSKill.h"


NS_BEGIN(Client)
class CEdgFireBall : public CDragonSkill
{
public:
	DECLARE_DERIVED_TYPE(CEdgFireBall, CDragonSkill)

protected:
	explicit CEdgFireBall();
	explicit CEdgFireBall(const CEdgFireBall& rhs);
	~CEdgFireBall() override;

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
	void			MoveBall(_float fTimeDelta);
	_bool			MoveSweep(_vector vNextPos);
public:
	static E::UPtr<CEdgFireBall> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END

