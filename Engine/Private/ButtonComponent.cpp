#include "pch.h"

#include "ButtonComponent.h"
#include "UIObject.h"

NS_USING(Engine)

CButtonComponent::CButtonComponent()
{
}

CButtonComponent::~CButtonComponent()
{
}

bool CButtonComponent::CheckPixelPerfectCollision(_float2 mousePos, bool bIsTopUI)
{
	CUIObject* pOwner = static_cast<CUIObject*>(m_pGameObject);
	if (!pOwner) return false;

	bool bCurrentCollision = bIsTopUI && PtInRect(mousePos);

	if (!m_bIsHovered && bCurrentCollision) 
	{
		m_bIsHovered = true;
		pOwner->PlayEffect(ETOUI(CUIObject::UI_STATE::ENTER));
	}
	else if (m_bIsHovered && !bCurrentCollision)
	{
		m_bIsHovered = false;
		pOwner->PlayEffect(ETOUI(CUIObject::UI_STATE::EXIT));
	}
	else if (bCurrentCollision)
	{
		pOwner->PlayEffect(ETOUI(CUIObject::UI_STATE::HOVERED));
	}

	if (m_bIsHovered && CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB))
	{
		pOwner->PlayEffect(ETOUI(CUIObject::UI_STATE::CLICK));
	}

	return false;
}

bool CButtonComponent::PtInRect(_float2 mousePos)
{
	CUIObject* pOwner = static_cast<CUIObject*>(m_pGameObject);
	const UI_INFO& selectInfo = pOwner->GetUIInfo();

	_float2 origin = { selectInfo.fX, selectInfo.fY };
	_float2 size = { selectInfo.SizeX, selectInfo.SizeY };

	_float2 minPos =
	{
		origin.x - size.x * 0.5f,
		origin.y - size.y * 0.5f
	};

	_float2 maxPos =
	{
		origin.x + size.x * 0.5f,
		origin.y + size.y * 0.5f
	};

	if (mousePos.x >= minPos.x &&
		mousePos.x <= maxPos.x &&
		mousePos.y >= minPos.y &&
		mousePos.y <= maxPos.y)
	{
		return true;
	}

	return false;
}

HRESULT CButtonComponent::Initialize(void* pArg)
{
	if (FAILED(CUIComponent::Initialize(pArg)))
		return E_FAIL;


	return S_OK;
}

void CButtonComponent::Update(_float fTimeDelta, _float2 mousePos)
{
	CUIComponent::Update(fTimeDelta);

	CheckPixelPerfectCollision(mousePos, true);
}

UPtr<CButtonComponent> CButtonComponent::Create()
{
	auto pInstance = ToUPtr(new CButtonComponent{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CButtonComponent");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CButtonComponent::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CButtonComponent{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned: CButtonComponent");
		return nullptr;
	}
	return pInstance;
}
