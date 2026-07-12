#include "pch.h"
#include "GameInstance.h"
#include "TextUI.h"

CTextUI::CTextUI()
{
}

CTextUI::~CTextUI()
{
}

HRESULT CTextUI::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CTextUI::TEXT_DESC*>(pArg);

	m_textInfo.Text = pDesc->Text;

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;

	return S_OK;
}

void CTextUI::Update(_float fTimeDelta)
{
	CUIObject::Update(fTimeDelta);

	auto clientSize = CGameInstance::Get().GetClientScreenSize();

	CGameInstance::Get().FontAddLateDraw(RENDERGROUP::UI, "NeoDGM_15px", m_textInfo.Text.c_str(), { m_UIINFO.fX, m_UIINFO.fY }, m_UIINFO.SizeX, XMVectorSet(1.f, 1.f, 1.f, m_UIINFO.Alpha));
}

void CTextUI::PlayEffect(uint32_t uiState)
{
}
