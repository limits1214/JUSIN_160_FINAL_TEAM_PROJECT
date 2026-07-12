#include "pch.h"
#include "TextBox.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"

NS_USING(Client)

CTextBox::CTextBox()
{

}

CTextBox::~CTextBox()
{
}

HRESULT CTextBox::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CTextUI::TEXT_DESC*>(pArg);

	if (FAILED(CTextUI::Initialize(pDesc)))
		return E_FAIL;

	m_UIINFO.UIType = ETOUI(UI_TYPE::TEXUI);

	return S_OK;
}

void CTextBox::PriorityUpdate(E::_float fTimeDelta)
{
	return;
}

void CTextBox::Update(E::_float fTimeDelta)
{
	CTextUI::Update(fTimeDelta);
}

void CTextBox::LateUpdate(E::_float fTimeDelta)
{
	return;
}

HRESULT CTextBox::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	return S_OK;
}

E::UPtr<CTextBox> CTextBox::Create()
{
	auto pInstance = E::ToUPtr(new CTextBox{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CTexUI");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CTextBox::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CTextBox{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTextureUI");
		return nullptr;
	}

	return pInstance;
}
