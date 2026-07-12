#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL CUIComponent : public CComponent
{
public:
	DECLARE_DERIVED_TYPE(CUIComponent, CComponent)

protected:
	explicit CUIComponent();
	~CUIComponent() override;

public:
	virtual HRESULT Initialize(void* pArg) override;
	virtual void	Update(_float fTimeDelta);
	virtual void	Update(_float fTimeDelta, _float2 mousePos);
};

NS_END
