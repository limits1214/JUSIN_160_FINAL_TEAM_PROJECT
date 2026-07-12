#include "pch.h"

#include "UIComponent.h"

NS_USING(Engine)

CUIComponent::CUIComponent()
{
}


CUIComponent::~CUIComponent()
{
}

HRESULT CUIComponent::Initialize(void* pArg)
{
	auto		pDesc = static_cast<tagComponentDesc*>(pArg);

	if (FAILED(CComponent::Initialize(pArg)))
			return E_FAIL;

	return S_OK;
}

void CUIComponent::Update(_float fTimeDelta)
{
}

void CUIComponent::Update(_float fTimeDelta, _float2 mousePos)
{
}
