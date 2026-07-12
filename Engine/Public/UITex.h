#pragma once

#include "GameObject.h"
#include "UIObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL CUITex : public CUIObject
{
public:
	DECLARE_DERIVED_TYPE(CUITex, CUIObject)

protected:
	CUITex();
	~CUITex() override;

protected:
	virtual void PlayEffect(uint32_t uiState) override;

public:
	HRESULT Initialize(void* pArg) override;
	virtual void Update(_float fTimeDelta);
	virtual void LateUpdate(_float fTimeDelta);
};

NS_END

